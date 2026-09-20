#include <Arduino.h>
#include <math.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Ui.h"

// ---------- État de l'interface ----------
enum Ecran { ECRAN_MENU, ECRAN_APP };

struct ItemMenu { const char *nom; uint16_t couleur; };

static ItemMenu menu[] = {
  { "Compteur", TFT_CYAN   },
  { "Infos",    TFT_GREEN  },
  { "Reglages", TFT_ORANGE },
};
static const uint8_t NB_ITEMS = sizeof(menu) / sizeof(menu[0]);

static Ecran    ecranActuel    = ECRAN_MENU;
static int8_t   selection      = 0;
static int8_t   appActive      = 0;
static int      valeurCompteur = 0;
static uint32_t debutAppui     = 0;

static ButtonTracker bouton;

// ---------- Mise en page ----------
static const int MARGE  = 12;
static const int H_CASE = 80;
static const int ESPACE = 10;
static const int Y0     = 40;
static const int PAS    = H_CASE + ESPACE;

static float yHighlight = -1;     // Position animée du surlignage

// ---------- Dessin ----------
static void dessinerEntete(const char *titre) {
  spr.fillRect(0, 0, spr.width(), 28, TH.entete);
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TH.texte, TH.entete);
  spr.setTextDatum(middle_center);
  spr.drawString(titre, spr.width() / 2, 14);
}

static bool animationEnCours() {
  return fabs((Y0 + selection * PAS) - yHighlight) > 0.5f;
}

// Barre de progression de l'appui — dessine seulement, aucune logique d'état
static void dessinerBarreAppui(const ButtonTracker::State &btn) {
  if (!btn.isPressed) return;

  uint32_t duree = millis() - debutAppui;

  uint16_t couleur;
  if      (duree < SHORT_PRESS_INTERVAL) couleur = TFT_GREEN;    // Clic
  else if (duree < LONG_PRESS_INTERVAL)  couleur = TFT_ORANGE;   // Moyen
  else                                   couleur = TFT_RED;      // Long

  float ratio = min((float)duree / LONG_PRESS_INTERVAL, 1.0f);

  const int H = 6;
  const int Y = spr.height() - H;

  spr.fillRect(0, Y, spr.width(), H, TH.entete);
  spr.fillRect(0, Y, spr.width() * ratio, H, couleur);

  // Repère du seuil "moyen"
  int xSeuil = spr.width() * ((float)SHORT_PRESS_INTERVAL / LONG_PRESS_INTERVAL);
  spr.drawFastVLine(xSeuil, Y, H, TH.texte);
}

static void dessinerMenu() {
  const int w = spr.width() - 2 * MARGE;

  float cible = Y0 + selection * PAS;
  if (yHighlight < 0) yHighlight = cible;
  yHighlight += (cible - yHighlight) * 0.3f;

  spr.fillSprite(TH.fond);
  dessinerEntete("ATS-OS");

  spr.setFont(&fonts::Font4);
  spr.setTextDatum(middle_center);

  for (uint8_t i = 0; i < NB_ITEMS; i++) {
    int y = Y0 + i * PAS;
    spr.drawRoundRect(MARGE, y, w, H_CASE, 8, menu[i].couleur);
    spr.setTextColor(menu[i].couleur);
    spr.drawString(menu[i].nom, spr.width() / 2, y + H_CASE / 2);
  }

  spr.fillRoundRect(MARGE, (int)yHighlight, w, H_CASE, 8, menu[selection].couleur);
  spr.setTextColor(TH.fond);
  spr.drawString(menu[selection].nom, spr.width() / 2, (int)yHighlight + H_CASE / 2);
}

static void dessinerApp() {
  spr.fillSprite(TH.fond);
  dessinerEntete(menu[appActive].nom);
  spr.setTextDatum(middle_center);

  switch (appActive) {
    case 0:   // Compteur
      spr.setFont(&fonts::Font7);
      spr.setTextColor(menu[0].couleur, TH.fond);
      spr.drawString(String(valeurCompteur), spr.width() / 2, 150);
      break;

    case 1:   // Infos
      spr.setFont(&fonts::Font2);
      spr.setTextColor(TH.texte, TH.fond);
      spr.drawString("ESP32-S3 @240MHz", spr.width() / 2, 110);
      spr.drawString(String(ESP.getFreeHeap()  / 1024) + " ko RAM",   spr.width() / 2, 140);
      spr.drawString(String(ESP.getFreePsram() / 1024) + " ko PSRAM", spr.width() / 2, 170);
      spr.drawString("Uptime " + String(millis() / 1000) + "s",       spr.width() / 2, 200);
      break;

    case 2:   // Réglages
      spr.setFont(&fonts::Font2);
      spr.setTextColor(TH.texte, TH.fond);
      spr.drawString("A venir...", spr.width() / 2, 150);
      break;
  }

  spr.setFont(&fonts::Font0);
  spr.setTextColor(TFT_DARKGREY, TH.fond);
  spr.drawString("Clic=RAZ  Moyen=retour", spr.width() / 2, spr.height() - 22);
}

// ---------- API publique ----------
void uiInit() {
  dessinerMenu();
  displayPush();
}

void uiUpdate() {
  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  // Détection du front d'appui — tourne à CHAQUE cycle, hors du rendu
  static bool etaitPresse = false;
  if (btn.isPressed && !etaitPresse) debutAppui = millis();
  bool vientDeRelacher = (etaitPresse && !btn.isPressed);
  etaitPresse = btn.isPressed;

  int  delta       = encoderGetDelta();
  bool aRedessiner = false;

  // Appui long : retour au menu principal
  static bool longTraite = false;
  if (btn.isLongPressed && !longTraite) {
    longTraite  = true;
    ecranActuel = ECRAN_MENU;
    aRedessiner = true;
  }
  if (!btn.isPressed) longTraite = false;

  if (ecranActuel == ECRAN_MENU) {
    if (delta != 0 && !animationEnCours()) {
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
    if (btn.wasClicked && appActive == 0) {
      valeurCompteur = 0;
      aRedessiner = true;
    }
    if (delta != 0 && appActive == 0) {
      valeurCompteur += delta;
      aRedessiner = true;
    }
  }

  // Rafraîchissement périodique pour les écrans à contenu dynamique
  static uint32_t dernierRefresh = 0;
  bool refreshPeriodique = false;

  if (ecranActuel == ECRAN_APP && appActive == 1) {     // Écran Infos
    if (millis() - dernierRefresh >= 500) {            // 1 Hz suffit
      dernierRefresh = millis();
      refreshPeriodique = true;
    }
  }

  if (aRedessiner || animationEnCours() || btn.isPressed
      || vientDeRelacher || refreshPeriodique) {
    if (ecranActuel == ECRAN_MENU) dessinerMenu();
    else                           dessinerApp();

    dessinerBarreAppui(btn);
    displayPush();
  }
}