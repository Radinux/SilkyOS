#include "App.h"
#include "Display.h"      // ← ajout : apporte les TFT_*

// Déclarations des callbacks définis dans apps/
void compteurUpdate(int, const ButtonTracker::State &);
void compteurDraw();
void compteurExit();
void infosDraw();
void reglagesEnter();                                    // ← ajout
void reglagesUpdate(int, const ButtonTracker::State &);
void reglagesDraw();
void reglagesExit();

App apps[] = {
  //  nom         couleur      onEnter        onUpdate        onDraw        onExit        dynamique
  { "Compteur", TFT_CYAN,   nullptr,       compteurUpdate, compteurDraw, compteurExit, false },
  { "Infos",    TFT_GREEN,  nullptr,       nullptr,        infosDraw,    nullptr,      true  },
  { "Reglages", TFT_ORANGE, reglagesEnter, reglagesUpdate, reglagesDraw, reglagesExit, false },
};

const uint8_t NB_APPS = sizeof(apps) / sizeof(apps[0]);