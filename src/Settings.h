#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

enum TypeReglage {
  REG_TOGGLE,
  REG_VALEUR,
  REG_CHOIX,
  REG_ACTION,
  REG_SOUSMENU,     // ← ouvre un écran dédié
};

struct Reglage {
  const char   *nom;
  TypeReglage   type;
  void         *cible;
  int           min, max;
  const char  **options;
  uint8_t       nbOptions;
  void        (*action)();
  void        (*draw)();      // ← rendu du sous-écran
};

extern Reglage  listeReglages[];
extern const uint8_t NB_REGLAGES;

#endif