#include <WiFi.h>
#include <WiFiManager.h>
#include "Network.h"
#include "Storage.h"
#include <WebServer.h>
#include <ElegantOTA.h>

static WebServer serveur(80);
static bool serveurActif = false;
static WiFiManager wm;
static bool portailActif = false;

void netApply(uint8_t mode) {
  portailActif = false;

  switch (mode) {
    case ATS_WIFI_OFF:
      WiFi.disconnect(true);
      WiFi.mode(WIFI_MODE_NULL);
      Serial.println("[WiFi] Desactive");
      break;

    case ATS_WIFI_AP:
      WiFi.mode(WIFI_AP_STA);
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal("ATS-OS-Setup");
      portailActif = true;
      Serial.println("[WiFi] Portail : connecte-toi au reseau ATS-OS-Setup");
      break;

    case ATS_WIFI_STA:
      WiFi.mode(WIFI_STA);
      wm.setEnableConfigPortal(false);     // Pas de portail auto si echec
        if (wm.autoConnect()) {
        Serial.printf("[WiFi] Connecte a %s (%s)\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        ElegantOTA.begin(&serveur);      // page OTA sur /update
        serveur.begin();
        serveurActif = true;
        Serial.printf("[OTA] Disponible sur http://%s/update\n",
                      WiFi.localIP().toString().c_str());
      } else {
        Serial.println("[WiFi] Echec de connexion");
      }
      break;
  }
}

void netInit() {
  netApply(reglages.modeWifi);
}

// À appeler régulièrement quand le portail tourne
void netTick() {
  if (portailActif)  wm.process();
  if (serveurActif) {
    serveur.handleClient();
    ElegantOTA.loop();
  }
}

bool netIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String netGetIP() {
  if (portailActif)     return WiFi.softAPIP().toString();
  if (netIsConnected()) return WiFi.localIP().toString();
  return "-";
}

String netGetSSID() {
  if (portailActif)     return "ATS-OS-Setup";
  if (netIsConnected()) return WiFi.SSID();
  return "-";
}

const char *netGetStatusText() {
  if (portailActif)     return "Portail actif";
  if (netIsConnected()) return "Connecte";
  if (reglages.modeWifi == ATS_WIFI_OFF) return "Desactive";
  return "Deconnecte";
}