#include "Settings.h"
#include "Storage.h"
#include "Display.h"

// Options du mode WiFi (prêt pour la suite)
static const char *optionsWifi[] = { "OFF", "AP", "Box" };

// Actions
static void actionReset() {
  storageReset();
}

Reglage listeReglages[] = {
  { "Luminosite",  REG_VALEUR, &reglages.luminosite,   0, 255, nullptr, 0, nullptr },
  { "Sens encodeur", REG_TOGGLE, &reglages.sensEncodeur, 0, 0,  nullptr, 0, nullptr },
  { "WiFi",        REG_CHOIX,  &reglages.modeWifi,     0, 2, optionsWifi, 3, nullptr },
  { "Reinitialiser", REG_ACTION, nullptr,              0, 0,  nullptr, 0, actionReset },
};

const uint8_t NB_REGLAGES = sizeof(listeReglages) / sizeof(listeReglages[0]);