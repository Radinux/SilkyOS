#include <Arduino.h>
#include <lvgl.h>
#include "Config.h"
#include "Diag.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"
#include "Battery.h"
#include "Meteo.h"
#include "Network.h"
#include "Lvgl.h"
#include "Ui.h"

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) delay(10);   // Laisse le moniteur se reconnecter

  Serial.println("=== SilkyOS boot ===");
  Serial.printf("Dernier reset : %s\n", diagRaisonReset());

  storageInit();        // Réglages NVS (avant tout le reste)
  displayInit();        // Écran + rétroéclairage
  encoderInit();        // Encodeur + bouton
  batteryInit();        // Première mesure de la batterie
  meteoInit();          // Mutex + ville sauvegardée (AVANT la tâche réseau qui s'en sert)
  netInit();            // Tâche réseau sur le cœur 0
  lvglInit();           // LVGL : affichage, thème, entrées
  uiInit();             // Démarrage puis launcher
}

void loop() {
  lv_timer_handler();   // Moteur LVGL : rendu, animations, lecture encodeur
  uiUpdate();           // Interface : barres, veille, apps en fond, navigation
  batteryTick();        // Mesure batterie (5 fois par seconde)

  // En veille, la boucle tourne 10 fois moins vite
  delay(uiEnVeille() ? 50 : 5);
}