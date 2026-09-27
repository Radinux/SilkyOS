#include "App.h"
#include "Icones.h"

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
void radioCreate(lv_obj_t *contenu);
void radioExit();

const App apps[] = {
  //  nom          icone               couleur   onCreate        onExit        onFond
  { "Horloge",   ICO_HORLOGE,   0x5E5CE6, horlogeCreate,  horlogeExit,  nullptr      },
  { "Chrono",    ICO_CHRONO,    0x30D158, chronoCreate,   chronoExit,   nullptr      },
  { "Minuteur",  ICO_MINUTEUR,  0xFF375F, minuteurCreate, minuteurExit, minuteurFond },
  //{ "Radio",     ICO_RADIO,     0xBF5AF2, radioCreate,    radioExit,    nullptr      },
  { "Snake",     ICO_JEU,       0xFFD60A, snakeCreate,    snakeExit,    nullptr      },
  { "Meteo",     ICO_METEO,     0x64D2FF, meteoCreate,    meteoExit,    nullptr      },
  { "Reglages",  ICO_REGLAGES,  0xFF9800, reglagesCreate, reglagesExit, nullptr      },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);