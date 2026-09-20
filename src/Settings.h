#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

enum TypeReglage {
  REG_TOGGLE,    // Booléen ON/OFF
  REG_VALEUR,    // Entier dans une plage
  REG_CHOIX,     // Liste d'options
  REG_ACTION,    // Déclenche une fonction
};

struct Reglage {
  const char   *nom;
  TypeReglage   type;
  void         *cible;         // Pointeur vers la donnée à modifier
  int           min, max;      // Pour REG_VALEUR
  const char  **options;       // Pour REG_CHOIX
  uint8_t       nbOptions;
  void        (*action)();     // Pour REG_ACTION
};

extern Reglage  listeReglages[];
extern const uint8_t NB_REGLAGES;

#endif