#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Ui.h"

#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Ui.h"

enum Ecran { ECRAN_MENU, ECRAN_APP };

struct ItemMenu { const char *nom; uint16_t couleur; };

static ItemMenu menu[] = {
  { "Compteur", TFT_CYAN   },
  { "Infos",    TFT_GREEN  },
  { "Reglages", TFT_ORANGE },
};
static const uint8_t NB_ITEMS = sizeof(menu) / sizeof(menu[0]);

static Ecran  ecranActuel    = ECRAN_MENU;
static int8_t selection      = 0;
static int8_t appActive      = 0;
static int    valeurCompteur = 0;

static ButtonTracker bouton;

// ---------- Dessin ----------
static void dessinerEntete(const char *titre) {
  spr.fillRect(0, 0, spr.width(), 28, TH.entete);
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TH.texte, TH.entete);
  spr.setTextDatum(middle_center);
  spr.drawString(titre, spr.width() / 2, 14);
}

static void dessinerMenu() {
  spr.fillSprite(TH.fond);
  dessinerEntete("ATS-OS");

  const int MARGE = 12, H_CASE = 80, ESPACE = 10, Y0 = 40;

  for (uint8_t i = 0; i < NB_ITEMS; i++) {
    int y = Y0 + i * (H_CASE + ESPACE);
    int w = spr.width() - 2 * MARGE;

    if (i == selection) {
      spr.fillRoundRect(MARGE, y, w, H_CASE, 8, menu[i].couleur);
      spr.setTextColor(TH.fond);
    } else {
      spr.drawRoundRect(MARGE, y, w, H_CASE, 8, menu[i].couleur);
      spr.setTextColor(menu[i].couleur);
    }

    spr.setFont(&fonts::Font4);
    spr.setTextDatum(middle_center);
    spr.drawString(menu[i].nom, spr.width() / 2, y + H_CASE / 2);
  }

  displayPush();
}

static void dessinerApp() {
  spr.fillSprite(TH.fond);
  dessinerEntete(menu[appActive].nom);
  spr.setTextDatum(middle_center);

  switch (appActive) {
    case 0:
      spr.setFont(&fonts::Font7);
      spr.setTextColor(menu[0].couleur, TH.fond);
      spr.drawString(String(valeurCompteur), spr.width() / 2, 150);
      break;

    case 1:
      spr.setFont(&fonts::Font2);
      spr.setTextColor(TH.texte, TH.fond);
      spr.drawString("ESP32-S3 @240MHz", spr.width() / 2, 110);
      spr.drawString(String(ESP.getFreeHeap() / 1024) + " ko RAM",
                     spr.width() / 2, 140);
      spr.drawString(String(ESP.getFreePsram() / 1024) + " ko PSRAM",
                     spr.width() / 2, 170);
      spr.drawString("Uptime " + String(millis() / 1000) + "s",
                     spr.width() / 2, 200);
      break;

    case 2:
      spr.setFont(&fonts::Font2);
      spr.setTextColor(TH.texte, TH.fond);
      spr.drawString("A venir...", spr.width() / 2, 150);
      break;
  }

  spr.setFont(&fonts::Font0);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Moyen=retour  Long=menu", spr.width() / 2, spr.height() - 12);

  displayPush();
}

// ---------- API ----------
void uiInit() {
  dessinerMenu();
}

void uiUpdate() {
  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  int delta = encoderGetDelta();
  bool aRedessiner = false;

  static bool longTraite = false;
  if (btn.isLongPressed && !longTraite) {
    longTraite  = true;
    ecranActuel = ECRAN_MENU;
    aRedessiner = true;
  }
  if (!btn.isPressed) longTraite = false;

  if (ecranActuel == ECRAN_MENU) {
    if (delta != 0) {
      selection = (selection + delta) % NB_ITEMS;
      if (selection < 0) selection += NB_ITEMS;
      aRedessiner = true;
    }
    if (btn.wasClicked) {
      appActive   = selection;
      ecranActuel = ECRAN_APP;
      aRedessiner = true;
    }
  } else {
    if (btn.wasShortPressed) {
      ecranActuel = ECRAN_MENU;
      aRedessiner = true;
    }
    if (delta != 0 && appActive == 0) {
      valeurCompteur += delta;
      aRedessiner = true;
    }
  }

  if (aRedessiner) {
    if (ecranActuel == ECRAN_MENU) dessinerMenu();
    else                           dessinerApp();
  }
}