#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"      // ← présent ?
#include "Ui.h"

void setup() {
  Serial.begin(115200);
  delay(1000);            // ← laisse le temps à l'USB de s'établir

  Serial.println("=== ATS-OS boot ===");   // ← test : ça doit s'afficher

  storageInit();          // ← présent ?
  displayInit();
  encoderInit();
  uiInit();
}

void loop() {
  uiUpdate();
  
  static uint32_t t = 0;
  if (millis() - t > 2000) {
    t = millis();
    Serial.printf("alive - compteur=%d\n", reglages.compteur);
  }
  
  delay(5);
}