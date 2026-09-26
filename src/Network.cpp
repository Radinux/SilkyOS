#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "Network.h"
#include "Storage.h"
#include "Clock.h"

enum Etat : uint8_t { ETAT_OFF, ETAT_CONNEXION, ETAT_CONNECTE, ETAT_ECHEC, ETAT_PORTAIL };

// Écrit par la tâche réseau, lu par l'interface
static volatile uint8_t etat = ETAT_OFF;

// État de l'OTA : écrit par la tâche réseau, lu par l'interface
static volatile uint8_t otaEtat     = OTA_AUCUN;
static volatile uint8_t otaPourcent = 0;

// Utilisés UNIQUEMENT par la tâche réseau
static WiFiManager wm;
static WebServer   serveur(80);
static bool    portailActif = false;
static bool    serveurActif = false;
static bool    otaPret      = false;
static uint8_t modeActuel   = 255;

// File de demandes de mode (UI → tâche réseau)
static QueueHandle_t fileModes = nullptr;

// ---------- Côté tâche réseau ----------
static void arreterServices() {
  if (portailActif) { wm.stopConfigPortal(); portailActif = false; }
  if (serveurActif) { serveur.stop();        serveurActif = false; }
}

// Prépare ElegantOTA une seule fois, avec ses callbacks
static void preparerOta() {
  if (otaPret) return;

  ElegantOTA.begin(&serveur);

  // Ces callbacks tournent dans la tâche réseau : ils ne touchent PAS à LVGL,
  // ils se contentent de noter l'état pour que l'UI l'affiche.
  ElegantOTA.onStart([]() {
    otaPourcent = 0;
    otaEtat = OTA_EN_COURS;
  });

  ElegantOTA.onProgress([](size_t recu, size_t) {
    // Taille du nouveau firmware inconnue : on l'estime avec celle de l'actuel
    uint32_t estime = ESP.getSketchSize();
    uint32_t p = estime ? (uint64_t)recu * 100 / estime : 0;
    otaPourcent = p > 99 ? 99 : p;
  });

  ElegantOTA.onEnd([](bool succes) {
    otaPourcent = 100;
    otaEtat = succes ? OTA_REUSSI : OTA_ECHEC;
  });

  otaPret = true;
}

static void appliquerMode(uint8_t mode) {
  if (mode == modeActuel && etat != ETAT_ECHEC) return;   // Déjà dans ce mode
  modeActuel = mode;
  arreterServices();

  switch (mode) {
    case ATS_WIFI_OFF:
      WiFi.disconnect(true);
      WiFi.mode(WIFI_MODE_NULL);
      etat = ETAT_OFF;
      Serial.println("[WiFi] Desactive");
      break;

    case ATS_WIFI_AP:
      WiFi.mode(WIFI_AP_STA);
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal("SilkyOS-Setup");
      portailActif = true;
      etat = ETAT_PORTAIL;
      Serial.println("[WiFi] Portail : connecte-toi a SilkyOS-Setup");
      break;

    case ATS_WIFI_STA:
      etat = ETAT_CONNEXION;
      WiFi.mode(WIFI_STA);
      wm.setEnableConfigPortal(false);
      wm.setConnectTimeout(10);                // Abandon après 10 s

      if (wm.autoConnect()) {                  // Bloquant... mais seulement pour CETTE tâche
        preparerOta();
        clockDemarrerSynchro();              // Heure NTP dès qu'on a internet
        serveur.begin();
        serveurActif = true;
        etat = ETAT_CONNECTE;
        Serial.printf("[WiFi] Connecte a %s (%s)\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        Serial.printf("[OTA] http://%s/update\n", WiFi.localIP().toString().c_str());
      } else {
        etat = ETAT_ECHEC;
        Serial.println("[WiFi] Echec de connexion");
      }
      break;
  }
}

static void tacheReseau(void *) {
  for (;;) {
    uint8_t mode;
    // Attend une demande au plus 10 ms (ça sert aussi de pause), puis fait tourner les services
    if (xQueueReceive(fileModes, &mode, pdMS_TO_TICKS(10)) == pdTRUE) {
      appliquerMode(mode);
    }
    if (portailActif) wm.process();
    if (serveurActif) { serveur.handleClient(); ElegantOTA.loop(); }
  }
}

// ---------- API (appelée depuis l'interface) ----------
void netInit() {
  fileModes = xQueueCreate(1, sizeof(uint8_t));

  BaseType_t ok = xTaskCreatePinnedToCore(
      tacheReseau, "reseau",
      10240,          // Pile : WiFiManager est gourmand
      nullptr,
      1,              // Priorité
      nullptr,
      0);             // Cœur 0 : l'UI garde le cœur 1

  if (ok != pdPASS) Serial.println("[KO] Tache reseau non creee !");

  netApply(reglages.modeWifi);
}

void netApply(uint8_t mode) {
  // File de taille 1 : une nouvelle demande remplace celle pas encore traitée
  if (fileModes) xQueueOverwrite(fileModes, &mode);
}

bool netIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String netGetIP() {
  if (etat == ETAT_PORTAIL) return WiFi.softAPIP().toString();
  if (netIsConnected())     return WiFi.localIP().toString();
  return "-";
}

String netGetSSID() {
  if (etat == ETAT_PORTAIL) return "SilkyOS-Setup";
  if (netIsConnected())     return WiFi.SSID();
  return "-";
}

const char *netGetStatusText() {
  switch (etat) {
    case ETAT_PORTAIL:   return "Portail actif";
    case ETAT_CONNEXION: return "Connexion...";
    case ETAT_CONNECTE:  return netIsConnected() ? "Connecte" : "Deconnecte";
    case ETAT_ECHEC:     return "Echec";
    default:             return "Desactive";
  }
}

// ---------- État de l'OTA ----------
uint8_t netOtaEtat()      { return otaEtat; }
uint8_t netOtaPourcent()  { return otaPourcent; }
void    netOtaAcquitter() { otaEtat = OTA_AUCUN; }