#include <Arduino.h>
#include <math.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Storage.h"
#include "App.h"
#include "Ui.h"

// ---------- État de la navigation ----------
enum Ecran { ECRAN_MENU, ECRAN_APP };

static Ecran    ecranActuel = ECRAN_MENU;
static int8_t   selection   = 0;
static int8_t   appActive   = 0;
static uint32_t debutAppui  = 0;

static ButtonTracker bouton;

// ---------- Mise en page du menu ----------
static const int MARGE  = 12;
static const int H_CASE = 80;
static const int ESPACE = 10;
static const int Y0     = 40;
static const int PAS    = H_CASE + ESPACE;

static float yHighlight = -1;

// ---------- Chrome (éléments communs à tous les écrans) ----------
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

static void dessinerBarreAppui(const ButtonTracker::State &btn) {
  if (!btn.isPressed) return;

  uint32_t duree = millis() - debutAppui;

  uint16_t couleur;
  if      (duree < SHORT_PRESS_INTERVAL) couleur = TFT_GREEN;
  else if (duree < LONG_PRESS_INTERVAL)  couleur = TFT_ORANGE;
  else                                   couleur = TFT_RED;

  float ratio = min((float)duree / LONG_PRESS_INTERVAL, 1.0f);

  const int H = 6;
  const int Y = spr.height() - H;

  spr.fillRect(0, Y, spr.width(), H, TH.entete);
  spr.fillRect(0, Y, spr.width() * ratio, H, couleur);

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

  for (uint8_t i = 0; i < NB_APPS; i++) {
    int y = Y0 + i * PAS;
    spr.drawRoundRect(MARGE, y, w, H_CASE, 8, apps[i].couleur);
    spr.setTextColor(apps[i].couleur);
    spr.drawString(apps[i].nom, spr.width() / 2, y + H_CASE / 2);
  }

  spr.fillRoundRect(MARGE, (int)yHighlight, w, H_CASE, 8, apps[selection].couleur);
  spr.setTextColor(TH.fond);
  spr.drawString(apps[selection].nom, spr.width() / 2, (int)yHighlight + H_CASE / 2);
}

// ---------- Transitions ----------
static void ouvrirApp(int8_t index) {
  appActive   = index;
  ecranActuel = ECRAN_APP;
  if (apps[appActive].onEnter) apps[appActive].onEnter();
}

static void fermerApp() {
  if (apps[appActive].onExit) apps[appActive].onExit();
  ecranActuel = ECRAN_MENU;
}

// ---------- API publique ----------
void uiInit() {
  dessinerMenu();
  displayPush();
}

void uiUpdate() {
  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  static bool etaitPresse = false;
  if (btn.isPressed && !etaitPresse) debutAppui = millis();
  bool vientDeRelacher = (etaitPresse && !btn.isPressed);
  etaitPresse = btn.isPressed;

  int  delta       = encoderGetDelta();
  bool aRedessiner = false;

  // Appui long : retour au menu, quel que soit l'écran
  static bool longTraite = false;
  if (btn.isLongPressed && !longTraite) {
    longTraite = true;
    if (ecranActuel == ECRAN_APP) fermerApp();
    aRedessiner = true;
  }
  if (!btn.isPressed) longTraite = false;

  if (ecranActuel == ECRAN_MENU) {
    if (delta != 0 && !animationEnCours()) {
      selection = (selection + delta) % NB_APPS;
      if (selection < 0) selection += NB_APPS;
      aRedessiner = true;
    }
    if (btn.wasClicked) {
      ouvrirApp(selection);
      aRedessiner = true;
    }
  } else {
    // Sortie d'app gérée par le framework
    if (btn.wasShortPressed) {
      fermerApp();
      aRedessiner = true;
    } else {
      // L'app reçoit les entrées qui ne concernent pas la navigation
      if (apps[appActive].onUpdate) {
        apps[appActive].onUpdate(delta, btn);
        if (delta != 0 || btn.wasClicked) aRedessiner = true;
      }
    }
  }

  // Redessin périodique pour les apps dynamiques
  static uint32_t dernierRefresh = 0;
  bool refreshPeriodique = false;

  if (ecranActuel == ECRAN_APP && apps[appActive].dynamique) {
    if (millis() - dernierRefresh >= 500) {
      dernierRefresh = millis();
      refreshPeriodique = true;
    }
  }

  // Rendu
  if (aRedessiner || animationEnCours() || btn.isPressed
      || vientDeRelacher || refreshPeriodique) {

    if (ecranActuel == ECRAN_MENU) {
      dessinerMenu();
    } else {
      spr.fillSprite(TH.fond);
      dessinerEntete(apps[appActive].nom);
      if (apps[appActive].onDraw) apps[appActive].onDraw();
    }

    dessinerBarreAppui(btn);
    displayPush();
  }
}