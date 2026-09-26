#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Meteo.h"
#include "Network.h"
#include "PsramAlloc.h"

static const uint32_t PERIODE_MAJ = 30UL * 60 * 1000;   // Une mise à jour toutes les 30 min
static const uint32_t DELAI_ESSAI = 60UL * 1000;        // Après un échec : nouvel essai 1 min plus tard

// Partagé entre la tâche réseau (qui écrit) et l'UI (qui lit) : protégé par un mutex
static Meteo             donnees = {};
static SemaphoreHandle_t verrou  = nullptr;

// Utilisés uniquement dans la tâche réseau (page web et téléchargement tournent tous les deux dedans)
static float         lat = 0, lon = 0;
static bool          villeOk      = false;
static volatile bool majDemandee  = false;   // Posé par l'UI, lu par la tâche réseau
static uint32_t      dernierEssai = 0;
static bool          dejaEssaye   = false;

// ---------- Initialisation ----------
void meteoInit() {
  verrou = xSemaphoreCreateMutex();

  Preferences p;
  p.begin("meteo", true);                  // Espace NVS propre à la météo, en lecture seule
  p.getString("ville", donnees.ville, sizeof(donnees.ville));
  lat     = p.getFloat("lat", 0);
  lon     = p.getFloat("lon", 0);
  villeOk = p.getBool("ok", false);
  p.end();
}

// ---------- Accès protégés ----------
bool meteoGet(Meteo *copie) {
  xSemaphoreTake(verrou, portMAX_DELAY);   // On attend que personne n'écrive
  *copie = donnees;
  xSemaphoreGive(verrou);
  return copie->valide;
}

bool meteoVilleDefinie() { return villeOk; }

void meteoDemanderMaj() { majDemandee = true; }

void meteoDefinirVille(const char *nom, float la, float lo) {
  lat = la;
  lon = lo;
  villeOk = true;

  xSemaphoreTake(verrou, portMAX_DELAY);
  strlcpy(donnees.ville, nom, sizeof(donnees.ville));
  donnees.valide = false;                  // Les anciennes données étaient pour une autre ville
  xSemaphoreGive(verrou);

  Preferences p;
  p.begin("meteo", false);
  p.putString("ville", nom);
  p.putFloat("lat", la);
  p.putFloat("lon", lo);
  p.putBool("ok", true);
  p.end();

  majDemandee = true;
}

// ---------- Téléchargement (tâche réseau) ----------
static bool telecharger(Meteo &m) {
  char url[320];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min"
           "&timezone=auto&forecast_days=3",
           lat, lon);

  WiFiClientSecure client;
  client.setInsecure();                    // Données publiques : on ne vérifie pas le certificat
  HTTPClient http;
  http.setTimeout(8000);

  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[Meteo] Erreur HTTP %d\n", code);
    http.end();
    return false;
  }
  String corps = http.getString();
  http.end();

  JsonDocument doc(allocPsram());     // Les données JSON sont rangées en PSRAM
  if (deserializeJson(doc, corps)) {
    Serial.println("[Meteo] Reponse JSON invalide");
    return false;
  }

  JsonObject c = doc["current"];
  m.temp     = lroundf(c["temperature_2m"].as<float>());
  m.humidite = c["relative_humidity_2m"].as<uint8_t>();
  m.vent     = lroundf(c["wind_speed_10m"].as<float>());
  m.code     = c["weather_code"].as<uint8_t>();

  JsonObject d = doc["daily"];
  for (int i = 0; i < 3; i++) {
    m.tmin[i]     = lroundf(d["temperature_2m_min"][i].as<float>());
    m.tmax[i]     = lroundf(d["temperature_2m_max"][i].as<float>());
    m.codeJour[i] = d["weather_code"][i].as<uint8_t>();
  }
  return true;
}

void meteoTache() {
  if (!villeOk || !netIsConnected()) return;

  // Cette tâche est la seule à écrire "donnees" : elle peut les relire sans verrou
  bool perimee     = !donnees.valide || millis() - donnees.majMillis > PERIODE_MAJ;
  bool peutEssayer = !dejaEssaye || millis() - dernierEssai > DELAI_ESSAI;
  if (!majDemandee && !(perimee && peutEssayer)) return;

  majDemandee  = false;
  dejaEssaye   = true;
  dernierEssai = millis();

  Meteo m;
  xSemaphoreTake(verrou, portMAX_DELAY);
  m = donnees;                             // On part de l'existant (le nom de la ville...)
  xSemaphoreGive(verrou);

  if (!telecharger(m)) return;             // Téléchargement HORS du verrou : l'UI n'attend jamais
  m.valide    = true;
  m.majMillis = millis();

  xSemaphoreTake(verrou, portMAX_DELAY);
  donnees = m;                             // Mise à jour d'un seul coup, en quelques µs
  xSemaphoreGive(verrou);

  Serial.printf("[Meteo] %s : %d C, %s\n", m.ville, m.temp, meteoDescription(m.code));
}

// ---------- Codes météo WMO ----------
const char *meteoDescription(uint8_t c) {
  if (c == 0)  return "Ciel degage";
  if (c <= 2)  return "Peu nuageux";
  if (c == 3)  return "Couvert";
  if (c <= 48) return "Brouillard";
  if (c <= 57) return "Bruine";
  if (c <= 67) return "Pluie";
  if (c <= 77) return "Neige";
  if (c <= 82) return "Averses";
  if (c <= 86) return "Averses de neige";
  return "Orage";
}