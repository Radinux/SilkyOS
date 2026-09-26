#include <WiFi.h>
#include <ArduinoJson.h>
#include "Web.h"
#include "Battery.h"
#include "Config.h"
#include "Diag.h"
#include "Meteo.h"
#include "Network.h"

static WebServer *srv = nullptr;

// ---------- La page, stockée en flash ----------
static const char PAGE[] = R"rawliteral(
<!DOCTYPE html><html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SilkyOS</title>
<style>
:root{--fond:#000;--carte:#1c1c1e;--carte2:#2c2c2e;--texte:#fff;--texte2:#8e8e93;--accent:#0a84ff}
*{box-sizing:border-box}
body{margin:0 auto;max-width:720px;padding:16px;background:var(--fond);color:var(--texte);
     font-family:-apple-system,system-ui,sans-serif}
h1{font-size:30px;margin:8px 0 2px}h1 span{color:var(--accent)}
.sous{color:var(--texte2);margin-bottom:16px}
.grille{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}
.carte{background:var(--carte);border-radius:14px;padding:14px}
.titre{color:var(--texte2);font-size:13px;margin-bottom:4px}
.valeur{font-size:20px;font-weight:600}
.petit{color:var(--texte2);font-size:13px;margin-top:4px}
.meteo{margin-top:10px}
.temp{font-size:52px;font-weight:700}
.ligne{display:flex;gap:8px;margin-top:12px}
input{flex:1;padding:12px;border-radius:10px;border:0;background:var(--carte2);color:var(--texte);font-size:16px}
button{padding:12px 14px;border-radius:10px;border:0;background:var(--accent);color:#fff;font-size:15px;cursor:pointer}
.choix{display:block;width:100%;margin-top:6px;background:var(--carte2);text-align:left}
a{color:var(--accent)}
</style></head><body>
<h1>Silky<span>OS</span></h1><div class="sous" id="version">...</div>

<div class="grille">
 <div class="carte"><div class="titre">Batterie</div><div class="valeur" id="batt">-</div><div class="petit" id="volts"></div></div>
 <div class="carte"><div class="titre">WiFi</div><div class="valeur" id="wifi">-</div><div class="petit" id="rssi"></div></div>
 <div class="carte"><div class="titre">Mémoire</div><div class="valeur" id="ram">-</div><div class="petit" id="psram"></div></div>
 <div class="carte"><div class="titre">Allumé depuis</div><div class="valeur" id="uptime">-</div><div class="petit" id="reset"></div></div>
</div>

<div class="carte meteo">
 <div class="titre" id="ville">Météo</div>
 <div class="temp" id="temp">--</div>
 <div class="valeur" id="desc"></div>
 <div class="petit" id="details"></div>
 <div class="ligne">
  <input id="q" placeholder="Chercher une ville..." onkeydown="if(event.key==='Enter')chercher()">
  <button onclick="chercher()">Chercher</button>
 </div>
 <div id="resultats"></div>
 <div class="ligne"><button onclick="actualiser()">Actualiser la météo</button></div>
</div>

<p class="petit">Mise à jour du firmware : <a href="/update">/update</a></p>

<script>
const $ = id => document.getElementById(id);
const ko = o => Math.round(o / 1024);

function duree(s) {
  const h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60;
  return h + ' h ' + String(m).padStart(2, '0') + ' min';
}

async function maj() {
  try {
    const e = await (await fetch('/api/etat')).json();
    $('version').textContent = 'v' + e.version;
    $('batt').textContent   = e.batterie.usb ? 'Sur USB' : e.batterie.pct + ' %';
    $('volts').textContent  = e.batterie.volts.toFixed(2) + ' V';
    $('wifi').textContent   = e.wifi.ssid;
    $('rssi').textContent   = e.wifi.rssi + ' dBm  ·  ' + e.wifi.ip;
    $('ram').textContent    = ko(e.memoire.ram_totale - e.memoire.ram_libre) + ' / ' + ko(e.memoire.ram_totale) + ' ko';
    $('psram').textContent  = 'PSRAM ' + ko(e.memoire.psram_totale - e.memoire.psram_libre) + ' / ' + ko(e.memoire.psram_totale) + ' ko';
    $('uptime').textContent = duree(e.uptime);
    $('reset').textContent  = 'Dernier reset : ' + e.reset;

    const m = e.meteo;
    $('ville').textContent = m.ville || 'Météo';
    if (m.valide) {
      $('temp').textContent    = m.temp + ' °C';
      $('desc').textContent    = m.desc;
      $('details').textContent = 'Humidité ' + m.humidite + ' %  ·  Vent ' + m.vent +
                                 ' km/h  ·  il y a ' + Math.round(m.age / 60) + ' min';
    } else {
      $('temp').textContent    = '--';
      $('desc').textContent    = m.ville ? 'Téléchargement...' : 'Choisis une ville ci-dessous';
      $('details').textContent = '';
    }
  } catch (err) {}
}

// Recherche directement auprès d'Open-Meteo : SilkyOS ne reçoit que la ville choisie
async function chercher() {
  const q = $('q').value.trim();
  if (!q) return;
  const r = await fetch('https://geocoding-api.open-meteo.com/v1/search?count=5&language=fr&name='
                        + encodeURIComponent(q));
  const j = await r.json();
  const liste = $('resultats');
  liste.innerHTML = '';
  (j.results || []).forEach(v => {
    const b = document.createElement('button');
    b.className = 'choix';
    b.textContent = v.name + (v.admin1 ? ', ' + v.admin1 : '') + ' (' + v.country_code + ')';
    b.onclick = () => choisir(v.name, v.latitude, v.longitude);
    liste.appendChild(b);
  });
  if (!liste.children.length) liste.textContent = 'Aucun résultat';
}

async function choisir(nom, lat, lon) {
  // Les polices de SilkyOS n'ont pas d'accents : "Pont-à-Mousson" devient "Pont-a-Mousson"
  nom = nom.normalize('NFD').replace(/[\u0300-\u036f]/g, '').slice(0, 31);
  await fetch('/api/ville', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: new URLSearchParams({nom, lat, lon})
  });
  $('resultats').textContent = 'Ville enregistrée : ' + nom;
  setTimeout(maj, 3000);
}

async function actualiser() {
  await fetch('/api/actualiser', {method: 'POST'});
  setTimeout(maj, 3000);
}

maj();
setInterval(maj, 5000);
</script></body></html>
)rawliteral";

// ---------- Routes ----------
static void pageAccueil() {
  srv->send(200, "text/html; charset=utf-8", PAGE);
}

static void apiEtat() {
  JsonDocument doc;
  doc["version"] = SILKY_VERSION;
  doc["uptime"]  = millis() / 1000;
  doc["reset"]   = diagRaisonReset();

  JsonObject b = doc["batterie"].to<JsonObject>();
  b["usb"]   = batteryOnUsb();
  b["pct"]   = batteryPercent();
  b["volts"] = batteryVolts();

  JsonObject mem = doc["memoire"].to<JsonObject>();
  mem["ram_libre"]    = ESP.getFreeHeap();
  mem["ram_totale"]   = ESP.getHeapSize();
  mem["psram_libre"]  = ESP.getFreePsram();
  mem["psram_totale"] = ESP.getPsramSize();

  JsonObject w = doc["wifi"].to<JsonObject>();
  w["ssid"] = netGetSSID();
  w["ip"]   = netGetIP();
  w["rssi"] = WiFi.RSSI();

  Meteo m;
  bool ok = meteoGet(&m);
  JsonObject mt = doc["meteo"].to<JsonObject>();
  mt["ville"]  = m.ville;
  mt["valide"] = ok;
  if (ok) {
    mt["temp"]     = m.temp;
    mt["desc"]     = meteoDescription(m.code);
    mt["humidite"] = m.humidite;
    mt["vent"]     = m.vent;
    mt["age"]      = (millis() - m.majMillis) / 1000;
  }

  String json;
  serializeJson(doc, json);
  srv->send(200, "application/json", json);
}

static void apiVille() {
  if (!srv->hasArg("nom") || !srv->hasArg("lat") || !srv->hasArg("lon")) {
    srv->send(400, "text/plain", "Parametres manquants");
    return;
  }
  meteoDefinirVille(srv->arg("nom").c_str(), srv->arg("lat").toFloat(), srv->arg("lon").toFloat());
  srv->send(200, "application/json", "{\"ok\":true}");
}

static void apiActualiser() {
  meteoDemanderMaj();
  srv->send(200, "application/json", "{\"ok\":true}");
}

void webInit(WebServer &serveur) {
  srv = &serveur;
  serveur.on("/",               HTTP_GET,  pageAccueil);
  serveur.on("/api/etat",       HTTP_GET,  apiEtat);
  serveur.on("/api/ville",      HTTP_POST, apiVille);
  serveur.on("/api/actualiser", HTTP_POST, apiActualiser);
}