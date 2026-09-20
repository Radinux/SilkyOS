#include "../App.h"
#include "../Display.h"
#include "../Storage.h"

void compteurUpdate(int delta, const ButtonTracker::State &btn) {
  if (delta != 0)     reglages.compteur += delta;
  if (btn.wasClicked) reglages.compteur = 0;
}

void compteurDraw() {
  spr.setFont(&fonts::Font7);
  spr.setTextColor(TFT_CYAN, TH.fond);
  spr.setTextDatum(middle_center);
  spr.drawString(String(reglages.compteur), spr.width() / 2, 150);

  spr.setFont(&fonts::Font0);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
}

void compteurExit() {
  storageSave();
}