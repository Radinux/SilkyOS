#include "App.h"

void infosCreate(lv_obj_t *contenu);
void infosExit();
void reglagesCreate(lv_obj_t *contenu);

const App apps[] = {
  //  nom         icone               couleur   onCreate        onExit
  { "Infos",    LV_SYMBOL_LIST,     0x4CAF50, infosCreate,    infosExit },
  { "Reglages", LV_SYMBOL_SETTINGS, 0xFF9800, reglagesCreate, nullptr   },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);