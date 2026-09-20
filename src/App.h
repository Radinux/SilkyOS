#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "Button.h"

// Une app = un ensemble de callbacks. Le framework les appelle, l'app ne sait
// rien de la navigation ni du rendu global.
struct App {
  const char *nom;
  uint16_t    couleur;

  void (*onEnter)();                                      // Ouverture (peut être nullptr)
  void (*onUpdate)(int delta, const ButtonTracker::State &btn);
  void (*onDraw)();
  void (*onExit)();                                       // Fermeture (sauvegarde…)
  bool  dynamique;                                        // Redessin périodique ?
};

extern App apps[];
extern const uint8_t NB_APPS;

#endif