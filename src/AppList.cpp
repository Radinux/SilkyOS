#include "App.h"

void horlogeCreate(lv_obj_t *contenu);
void horlogeExit();
void chronoCreate(lv_obj_t *contenu);
void chronoExit();
void minuteurCreate(lv_obj_t *contenu);
void minuteurExit();
void minuteurFond();
void reglagesCreate(lv_obj_t *contenu);
void reglagesExit();
void snakeCreate(lv_obj_t *contenu);
void snakeExit();
void meteoCreate(lv_obj_t *contenu);
void meteoExit();

const App apps[] = {
  //  nom          icone               couleur   onCreate        onExit        onFond
  { "Horloge",   LV_SYMBOL_REFRESH,  0x5E5CE6, horlogeCreate,  horlogeExit,  nullptr      },
  { "Chrono",    LV_SYMBOL_PLAY,     0x30D158, chronoCreate,   chronoExit,   nullptr      },
  { "Minuteur",  LV_SYMBOL_BELL,     0xFF375F, minuteurCreate, minuteurExit, minuteurFond },
  { "Snake",     LV_SYMBOL_SHUFFLE,  0xFFD60A, snakeCreate,    snakeExit,    nullptr      },
  { "Meteo",     LV_SYMBOL_TINT,     0x64D2FF, meteoCreate,    meteoExit,    nullptr      },
  { "Reglages",  LV_SYMBOL_SETTINGS, 0xFF9800, reglagesCreate, reglagesExit, nullptr      },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);