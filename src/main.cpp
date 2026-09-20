#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Ui.h"

void setup() {
  Serial.begin(115200);
  displayInit();
  encoderInit();
  uiInit();
}

void loop() {
  uiUpdate();
  delay(5);
}