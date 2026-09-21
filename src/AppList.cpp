#include "App.h"

void reglagesCreate(lv_obj_t *contenu);
void reglagesExit();

const App apps[] = {
  //  nom         icone               couleur   onCreate        onExit
  { "Reglages", LV_SYMBOL_SETTINGS, 0xFF9800, reglagesCreate, reglagesExit },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);