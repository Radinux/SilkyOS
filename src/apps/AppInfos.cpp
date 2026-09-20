#include "../App.h"
#include "../Display.h"

void infosDraw() {
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TH.texte, TH.fond);
  spr.setTextDatum(middle_center);

  spr.drawString("ESP32-S3 @240MHz", spr.width() / 2, 110);
  spr.drawString(String(ESP.getFreeHeap()  / 1024) + " ko RAM",   spr.width() / 2, 140);
  spr.drawString(String(ESP.getFreePsram() / 1024) + " ko PSRAM", spr.width() / 2, 170);
  spr.drawString("Uptime " + String(millis() / 1000) + "s",       spr.width() / 2, 200);
}