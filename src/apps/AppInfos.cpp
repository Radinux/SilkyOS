#include "../App.h"
#include "../Display.h"

// Barre de remplissage avec libellé et valeurs
static void barreUsage(const char *nom, uint32_t utilise, uint32_t total, int y) {
  const int MARGE = 14;
  const int W     = spr.width() - 2 * MARGE;
  const int H     = 10;

  float ratio = total ? (float)utilise / total : 0;

  uint16_t couleur = TFT_GREEN;
  if      (ratio > 0.75f) couleur = TFT_RED;
  else if (ratio > 0.5f)  couleur = TFT_ORANGE;

  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_left);
  spr.setTextColor(TH.texte, TH.fond);
  spr.drawString(nom, MARGE, y);

  spr.setTextDatum(top_right);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString(String(utilise / 1024) + " / " + String(total / 1024) + " ko",
                 spr.width() - MARGE, y);

  int yBarre = y + 20;
  spr.drawRoundRect(MARGE, yBarre, W, H, 3, TFT_DARKGREY);
  spr.fillRoundRect(MARGE + 1, yBarre + 1, (W - 2) * ratio, H - 2, 2, couleur);
}

void infosDraw() {
  // --- Relevés système mis en cache : les interroger à chaque frame coûte cher ---
  static uint32_t dernierReleve = 0;
  static uint32_t heapTotal = 0, heapLibre = 0;
  static uint32_t psramTotal = 0, psramLibre = 0;

  if (heapTotal == 0 || millis() - dernierReleve > 500) {
    dernierReleve = millis();
    heapTotal  = ESP.getHeapSize();
    heapLibre  = ESP.getFreeHeap();
    psramTotal = ESP.getPsramSize();
    psramLibre = ESP.getFreePsram();
  }

  // --- Puce (valeurs constantes : lues une seule fois) ---
  static String modele = ESP.getChipModel();
  static String cpu    = String(ESP.getChipCores()) + " coeurs - "
                       + String(getCpuFrequencyMhz()) + " MHz";

  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_center);
  spr.setTextColor(TFT_CYAN, TH.fond);
  spr.drawString(modele, spr.width() / 2, 40);

  spr.setFont(&fonts::Font0);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString(cpu, spr.width() / 2, 62);

  // --- Mémoire ---
  barreUsage("RAM",   heapTotal  - heapLibre,  heapTotal,  90);
  barreUsage("PSRAM", psramTotal - psramLibre, psramTotal, 145);

  // --- Uptime ---
  uint32_t s = millis() / 1000;
  char uptime[16];
  snprintf(uptime, sizeof(uptime), "%02lu:%02lu:%02lu",
           s / 3600, (s / 60) % 60, s % 60);

  spr.setFont(&fonts::Font2);
  spr.setTextDatum(top_center);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Uptime", spr.width() / 2, 215);

  spr.setFont(&fonts::Font4);
  spr.setTextColor(TFT_GREEN, TH.fond);
  spr.drawString(uptime, spr.width() / 2, 237);
}