#include "App.h"

void horlogeCreate(lv_obj_t *contenu);
void horlogeExit();
void reglagesCreate(lv_obj_t *contenu);
void reglagesExit();

const App apps[] = {
  //  nom         icone               couleur   onCreate        onExit
  { "Horloge",  LV_SYMBOL_BELL,     0x5E5CE6, horlogeCreate,  horlogeExit  },
  { "Reglages", LV_SYMBOL_SETTINGS, 0xFF9800, reglagesCreate, reglagesExit },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);