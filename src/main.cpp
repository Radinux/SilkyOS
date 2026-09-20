#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"      // ← présent ?
#include "Ui.h"
#include "Network.h"

void setup() {
  Serial.begin(115200);
  delay(1000);            // ← laisse le temps à l'USB de s'établir

  Serial.println("=== ATS-OS boot ===");   // ← test : ça doit s'afficher

  storageInit();          // ← présent ?
  displayInit();
  encoderInit();
  netInit();          // après storageInit()
  uiInit();
}

void loop() {
  uiUpdate();
  netTick();          // Fait tourner le portail de config sans bloquer
  delay(5);
}