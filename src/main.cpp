#include <Arduino.h>
#include <lvgl.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"
#include "Network.h"
#include "Lvgl.h"
#include "Ui.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ATS-OS boot (LVGL) ===");

  storageInit();        // Réglages NVS (avant tout le reste)
  displayInit();        // Écran + rétroéclairage
  encoderInit();        // Encodeur + bouton
  netInit();            // Tâche réseau sur le cœur 0 (ne bloque plus le boot)
  lvglInit();           // LVGL : affichage, thème, entrées
  uiInit();             // Menu principal
}

void loop() {
  lv_timer_handler();   // Moteur LVGL : rendu, animations, lecture encodeur
  uiUpdate();           // Retour (appui moyen) / menu (appui long)
  delay(5);
}