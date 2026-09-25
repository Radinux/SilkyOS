#include <Arduino.h>
#include <lvgl.h>
#include <esp_system.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"
#include "Network.h"
#include "Lvgl.h"
#include "Ui.h"

// Traduit la raison du dernier redémarrage (elle survit au reset)
static const char *raisonReset() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "mise sous tension";
    case ESP_RST_SW:       return "redemarrage logiciel";
    case ESP_RST_PANIC:    return "PANIC (exception ou assert)";
    case ESP_RST_INT_WDT:  return "watchdog d'interruption";
    case ESP_RST_TASK_WDT: return "watchdog de tache";
    case ESP_RST_WDT:      return "autre watchdog";
    case ESP_RST_BROWNOUT: return "BROWNOUT (chute de tension)";
    default:               return "inconnue";
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) delay(10);   // Laisse le moniteur se reconnecter

  Serial.println("=== ATS-OS boot (LVGL) ===");
  Serial.printf("Dernier reset : %s\n", raisonReset());

  storageInit();        // Réglages NVS (avant tout le reste)
  displayInit();        // Écran + rétroéclairage
  encoderInit();        // Encodeur + bouton
  netInit();            // Tâche réseau sur le cœur 0
  lvglInit();           // LVGL : affichage, thème, entrées
  uiInit();             // Menu principal
}

void loop() {
  lv_timer_handler();   // Moteur LVGL : rendu, animations, lecture encodeur
  uiUpdate();           // Retour (appui moyen) / menu (appui long)
  delay(5);
}