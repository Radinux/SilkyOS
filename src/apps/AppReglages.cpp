#include "../App.h"
#include "../Display.h"
#include "../Storage.h"
#include "../Settings.h"

static int8_t selReglage  = 0;
static bool   modeEdition = false;

// Valeur d'un réglage, sous forme de texte
static String valeurReglage(const Reglage &r) {
  switch (r.type) {
    case REG_TOGGLE:
      // Cas particulier : le sens d'encodeur s'affiche CW / CCW
      if (r.cible == &reglages.sensEncodeur)
        return *(bool *)r.cible ? "CCW" : "CW";
      return *(bool *)r.cible ? "ON" : "OFF";

    case REG_VALEUR: return String(*(uint8_t *)r.cible);
    case REG_CHOIX:  return r.options[*(uint8_t *)r.cible];
    case REG_ACTION: return ">";
  }
  return "";
}

void reglagesDraw() {
  const int H_LIGNE = 34;
  const int ESPACE  = 4;
  const int Y_DEBUT = 40;
  const int MARGE_X = 8;

  spr.setFont(&fonts::Font2);

  for (uint8_t i = 0; i < NB_REGLAGES; i++) {
    int y      = Y_DEBUT + i * (H_LIGNE + ESPACE);
    int yTexte = y + H_LIGNE / 2;
    bool actif = (i == selReglage);

    if (actif) {
      uint16_t fond = modeEdition ? TFT_ORANGE : TH.entete;
      spr.fillRoundRect(MARGE_X, y, spr.width() - 2 * MARGE_X, H_LIGNE, 4, fond);
    }

    spr.setTextColor(actif && modeEdition ? TH.fond : TH.texte);

    spr.setTextDatum(middle_left);
    spr.drawString(listeReglages[i].nom, MARGE_X + 8, yTexte);

    spr.setTextDatum(middle_right);
    spr.drawString(valeurReglage(listeReglages[i]), spr.width() - MARGE_X - 8, yTexte);
  }
}

void reglagesUpdate(int delta, const ButtonTracker::State &btn) {
  Reglage &r = listeReglages[selReglage];

  if (modeEdition) {
    // --- Modification de la valeur ---
    if (delta != 0) {
      switch (r.type) {
        case REG_VALEUR: {
          int v = *(uint8_t *)r.cible + delta * 5;
          *(uint8_t *)r.cible = constrain(v, r.min, r.max);
          displaySetBrightness(reglages.luminosite);
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
    }
    if (btn.wasClicked) {
      modeEdition = false;
      storageSave();
    }
  } else {
    // --- Navigation dans la liste ---
    if (delta != 0) {
      selReglage = (selReglage + delta) % NB_REGLAGES;
      if (selReglage < 0) selReglage += NB_REGLAGES;
    }
    if (btn.wasClicked) {
      switch (r.type) {
        case REG_TOGGLE:
          *(bool *)r.cible = !*(bool *)r.cible;
          storageSave();
          break;
        case REG_ACTION:
          if (r.action) r.action();
          break;
        default:
          modeEdition = true;
          break;
      }
    }
  }
}

void reglagesEnter() {
  selReglage  = 0;
  modeEdition = false;      // Toujours repartir proprement
}

void reglagesExit() {
  storageSave();
}