#include <WiFi.h>
#include "../App.h"
#include "../Display.h"
#include "../Network.h"
#include "../Storage.h"

void wifiPageDraw() {
  spr.setTextDatum(top_center);

  // --- Titre de la page ---
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Infos WiFi", spr.width() / 2, 40);

  // --- Statut ---
  bool ok = netIsConnected();
  spr.setFont(&fonts::Font4);
  spr.setTextColor(ok ? TFT_GREEN : TFT_ORANGE, TH.fond);
  spr.drawString(netGetStatusText(), spr.width() / 2, 70);

  // --- Réseau ---
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Reseau", spr.width() / 2, 115);
  spr.setTextColor(TH.texte, TH.fond);
  spr.drawString(netGetSSID(), spr.width() / 2, 137);

  // --- Adresse IP ---
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Adresse IP", spr.width() / 2, 172);
  spr.setTextColor(TFT_CYAN, TH.fond);
  spr.drawString(netGetIP(), spr.width() / 2, 194);

  // --- Signal ---
  if (ok) {
    int rssi = WiFi.RSSI();
    spr.setTextColor(TFT_DARKGREY, TH.fond);
    spr.drawString("Signal", spr.width() / 2, 229);

    int barres = map(constrain(rssi, -90, -50), -90, -50, 0, 4);
    for (int i = 0; i < 4; i++) {
      int x = spr.width() / 2 - 26 + i * 14;
      int h = 6 + i * 5;
      spr.fillRect(x, 275 - h, 9, h, (i < barres) ? TFT_GREEN : TFT_DARKGREY);
    }

    spr.setFont(&fonts::Font0);
    spr.setTextColor(TFT_DARKGREY, TH.fond);
    spr.drawString(String(rssi) + " dBm", spr.width() / 2, 282);
  }

  // --- Aide ---
  spr.setFont(&fonts::Font0);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Clic = retour", spr.width() / 2, spr.height() - 20);
}