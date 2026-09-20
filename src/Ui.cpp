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

static Ecran  ecranActuel    = ECRAN_MENU;
static int8_t selection      = 0;
static int8_t appActive      = 0;
static int    valeurCompteur = 0;

static ButtonTracker bouton;

// ---------- Mise en page ----------
static const int MARGE  = 12;
static const int H_CASE = 80;
static const int ESPACE = 10;
static const int Y0     = 40;
static const int PAS    = H_CASE + ESPACE;

static float yHighlight = -1;     // Position animée du surlignage (-1 = non initialisé)

// ---------- Dessin ----------
static void dessinerEntete(const char *titre) {
  spr.fillRect(0, 0, spr.width(), 28, TH.entete);
  spr.setFont(&fonts::Font2);
  spr.setTextColor(TH.texte, TH.entete);
  spr.setTextDatum(middle_center);
  spr.drawString(titre, spr.width() / 2, 14);
}

// Vrai tant que le surlignage n'a pas rejoint sa cible
static bool animationEnCours() {
  return fabs((Y0 + selection * PAS) - yHighlight) > 0.5f;
}

static void dessinerMenu() {
  const int w = spr.width() - 2 * MARGE;

  // Le surlignage rattrape progressivement la sélection
  float cible = Y0 + selection * PAS;
  if (yHighlight < 0) yHighlight = cible;          // 1er affichage : pas d'animation
  yHighlight += (cible - yHighlight) * 0.3f;       // Easing : 30% de la distance par frame

  spr.fillSprite(TH.fond);
  dessinerEntete("ATS-OS");

  spr.setFont(&fonts::Font4);
  spr.setTextDatum(middle_center);

  // Passe 1 : tous les items en version "inactive" (contour seul)
  for (uint8_t i = 0; i < NB_ITEMS; i++) {
    int y = Y0 + i * PAS;
    spr.drawRoundRect(MARGE, y, w, H_CASE, 8, menu[i].couleur);
    spr.setTextColor(menu[i].couleur);
    spr.drawString(menu[i].nom, spr.width() / 2, y + H_CASE / 2);
  }

  // Passe 2 : le surlignage plein, par-dessus, à sa position animée
  spr.fillRoundRect(MARGE, (int)yHighlight, w, H_CASE, 8, menu[selection].couleur);
  spr.setTextColor(TH.fond);
  spr.drawString(menu[selection].nom, spr.width() / 2, (int)yHighlight + H_CASE / 2);

  displayPush();
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
  spr.drawString("Clic=RAZ  Moyen=retour", spr.width() / 2, spr.height() - 12);

  displayPush();
}

// ---------- API publique ----------
void uiInit() {
  dessinerMenu();
}

void uiUpdate() {
  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

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
      valeurCompteur = 0;              // Clic = remise à zéro du compteur
      aRedessiner = true;
    }
    if (delta != 0 && appActive == 0) {
      valeurCompteur += delta;
      aRedessiner = true;
    }
  }

  if (aRedessiner || animationEnCours()) {
    if (ecranActuel == ECRAN_MENU) dessinerMenu();
    else                           dessinerApp();
  }
}