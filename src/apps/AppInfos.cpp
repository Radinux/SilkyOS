#include "../App.h"
#include "../Display.h"

// Barre de remplissage avec libellé et valeurs
static void barreUsage(const char *nom, uint32_t utilise, uint32_t total, int y) {
  const int MARGE = 14;
  const int W     = spr.width() - 2 * MARGE;
  const int H     = 10;

  float ratio = total ? (float)utilise / total : 0;

  // Couleur selon le taux d'occupation
  uint16_t couleur = TFT_GREEN;
  if (ratio > 0.75f) couleur = TFT_RED;
  else if (ratio > 0.5f) couleur = TFT_ORANGE;

  // Libellé + chiffres
  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_left);
  spr.setTextColor(TH.texte, TH.fond);
  spr.drawString(nom, MARGE, y);

  spr.setTextDatum(top_right);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString(String(utilise / 1024) + " / " + String(total / 1024) + " ko",
                 spr.width() - MARGE, y);

  // Barre
  int yBarre = y + 20;
  spr.drawRoundRect(MARGE, yBarre, W, H, 3, TFT_DARKGREY);
  spr.fillRoundRect(MARGE + 1, yBarre + 1, (W - 2) * ratio, H - 2, 2, couleur);
}

void infosDraw() {
  uint32_t heapTotal  = ESP.getHeapSize();
  uint32_t heapLibre  = ESP.getFreeHeap();
  uint32_t psramTotal = ESP.getPsramSize();
  uint32_t psramLibre = ESP.getFreePsram();
  uint32_t flashTotal = ESP.getFlashChipSize();
  uint32_t croquis    = ESP.getSketchSize();

  // --- En-tête : puce ---
  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_center);
  spr.setTextColor(TFT_CYAN, TH.fond);
  spr.drawString(ESP.getChipModel(), spr.width() / 2, 38);

  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.setFont(&fonts::Font0);
  spr.drawString(String(ESP.getChipCores()) + " coeurs - "
                 + String(getCpuFrequencyMhz()) + " MHz",
                 spr.width() / 2, 60);

  // --- Barres d'occupation ---
  barreUsage("RAM",   heapTotal  - heapLibre,  heapTotal,  80);
  barreUsage("PSRAM", psramTotal - psramLibre, psramTotal, 130);
  barreUsage("Flash", croquis,                 flashTotal, 180);

  // --- Uptime ---
  uint32_t s = millis() / 1000;
  char uptime[24];
  snprintf(uptime, sizeof(uptime), "%02lu:%02lu:%02lu",
           s / 3600, (s / 60) % 60, s % 60);

  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_center);
  spr.setTextColor(TH.texte, TH.fond);
  spr.drawString("Uptime", spr.width() / 2, 230);

  spr.setFont(&fonts::Font4);
  spr.setTextColor(TFT_GREEN, TH.fond);
  spr.drawString(uptime, spr.width() / 2, 252);
}