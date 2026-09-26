#ifndef APP_H
#define APP_H

#include <lvgl.h>

struct App {
  const char *nom;
  const char *icone;                      // Symbole LVGL (LV_SYMBOL_...)
  uint32_t    couleur;                    // 0xRRGGBB
  void (*onCreate)(lv_obj_t *contenu);    // Construit les widgets dans "contenu"
  void (*onExit)();                       // Nettoyage (timers, sauvegarde), peut être nullptr
  void (*onFond)();                       // Travail en arrière-plan, appelé en permanence (peut être nullptr)
};

extern const App     apps[];
extern const uint8_t NB_APPS;

#endif