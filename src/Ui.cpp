#include <Arduino.h>
#include <math.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Ui.h"
#include "Storage.h"
#include "Settings.h"

// ---------- État de l'interface ----------
enum Ecran { ECRAN_MENU, ECRAN_APP };

struct ItemMenu { const char *nom; uint16_t couleur; };

static ItemMenu menu[] = {
  { "Compteur", TFT_CYAN   },
  { "Infos",    TFT_GREEN  },
  { "Reglages", TFT_ORANGE },
};
static const uint8_t NB_ITEMS = sizeof(menu) / sizeof(menu[0]);

static Ecran    ecranActuel = ECRAN_MENU;
static int8_t   selection   = 0;
static int8_t selReglage = 0;        // Ligne sélectionnée dans Réglages
static bool   modeEdition = false;   // true = on modifie la valeur
static int8_t   appActive   = 0;
static uint32_t debutAppui  = 0;

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

// Affiche la valeur d'un réglage sous forme de texte
static String valeurReglage(const Reglage &r) {
  switch (r.type) {
    case REG_TOGGLE:
      return *(bool *)r.cible ? "ON" : "OFF";
    case REG_VALEUR:
      return String(*(uint8_t *)r.cible);
    case REG_CHOIX:
      return r.options[*(uint8_t *)r.cible];
    case REG_ACTION:
      return ">";
  }
  return "";
}

static void dessinerReglages() {
  const int H_LIGNE = 34;
  const int ESPACE  = 4;
  const int Y_DEBUT = 40;
  const int MARGE_X = 8;

  spr.setFont(&fonts::Font2);

  for (uint8_t i = 0; i < NB_REGLAGES; i++) {
    int y = Y_DEBUT + i * (H_LIGNE + ESPACE);
    int yTexte = y + H_LIGNE / 2;              // Centre vertical de la ligne
    bool actif = (i == selReglage);

    if (actif) {
      uint16_t fond = modeEdition ? TFT_ORANGE : TH.entete;
      spr.fillRoundRect(MARGE_X, y, spr.width() - 2 * MARGE_X, H_LIGNE, 4, fond);
    }

    spr.setTextColor(actif && modeEdition ? TH.fond : TH.texte);

    // Nom à gauche
    spr.setTextDatum(middle_left);
    spr.drawString(listeReglages[i].nom, MARGE_X + 8, yTexte);

    // Valeur à droite
    spr.setTextDatum(middle_right);
    spr.drawString(valeurReglage(listeReglages[i]),
                   spr.width() - MARGE_X - 8, yTexte);
  }
}

static void dessinerApp() {
  spr.fillSprite(TH.fond);
  dessinerEntete(menu[appActive].nom);
  spr.setTextDatum(middle_center);

  switch (appActive) {
    case 0:   // Compteur
      spr.setFont(&fonts::Font7);
      spr.setTextColor(menu[0].couleur, TH.fond);
      spr.drawString(String(reglages.compteur), spr.width() / 2, 150);
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
      dessinerReglages();
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

  // Appui long : sauvegarde + retour au menu principal
  static bool longTraite = false;
  if (btn.isLongPressed && !longTraite) {
    longTraite  = true;
    storageSave();
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
    
    } else if (appActive == 2) {
    // ===== Écran Réglages =====
    Reglage &r = listeReglages[selReglage];

    if (modeEdition) {
      // --- Modification de la valeur ---
      if (delta != 0) {
        switch (r.type) {
          case REG_VALEUR: {
            int v = *(uint8_t *)r.cible + delta * 5;    // Pas de 5
            *(uint8_t *)r.cible = constrain(v, r.min, r.max);
            displaySetBrightness(reglages.luminosite);  // ← ICI
            break;
          }
          case REG_CHOIX: {
            int v = (*(uint8_t *)r.cible + delta) % r.nbOptions;
            if (v < 0) v += r.nbOptions;
            *(uint8_t *)r.cible = v;
            break;
          }
          default: break;
        }
        aRedessiner = true;
      }
      if (btn.wasClicked) {              // Valider
        modeEdition = false;
        storageSave();
        aRedessiner = true;
      }
    } else {
      // --- Navigation dans la liste ---
      if (delta != 0) {
        selReglage = (selReglage + delta) % NB_REGLAGES;
        if (selReglage < 0) selReglage += NB_REGLAGES;
        aRedessiner = true;
      }
      if (btn.wasClicked) {
        switch (r.type) {
          case REG_TOGGLE:
            *(bool *)r.cible = !*(bool *)r.cible;    // Bascule直接
            storageSave();
            break;
          case REG_ACTION:
            if (r.action) r.action();
            break;
          default:
            modeEdition = true;                       // Entre en édition
            break;
        }
        aRedessiner = true;
      }
      if (btn.wasShortPressed) {
        storageSave();
        ecranActuel = ECRAN_MENU;
        aRedessiner = true;
      }
    }

  } else {
    // ===== Autres apps (Compteur, Infos) =====
    if (btn.wasShortPressed) {
      storageSave();
      ecranActuel = ECRAN_MENU;
      aRedessiner = true;
    }
    if (btn.wasClicked && appActive == 0) {
      reglages.compteur = 0;
      aRedessiner = true;
    }
    if (delta != 0 && appActive == 0) {
      reglages.compteur += delta;
      aRedessiner = true;
    }
  }

  // Rafraîchissement périodique pour les écrans à contenu dynamique
  static uint32_t dernierRefresh = 0;
  bool refreshPeriodique = false;

  if (ecranActuel == ECRAN_APP && appActive == 1) {     // Écran Infos
    if (millis() - dernierRefresh >= 500) {
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