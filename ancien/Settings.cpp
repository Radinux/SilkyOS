#include "Settings.h"
#include "Storage.h"
#include "Display.h"

// Options du mode WiFi (prêt pour la suite)
static const char *optionsWifi[] = { "OFF", "AP", "Box" };

// Actions
static void actionReset() {
  storageReset();
}

void wifiPageDraw();     // défini dans AppReglages.cpp ou un fichier dédié

Reglage listeReglages[] = {
  { "Luminosite",    REG_VALEUR,   &reglages.luminosite,   0, 255, nullptr, 0, nullptr, nullptr },
  { "Sens enco",     REG_TOGGLE,   &reglages.sensEncodeur, 0, 0,   nullptr, 0, nullptr, nullptr },
  { "Mode WiFi",     REG_CHOIX,    &reglages.modeWifi,     0, 2, optionsWifi, 3, nullptr, nullptr },
  { "Infos WiFi",    REG_SOUSMENU, nullptr,                0, 0,   nullptr, 0, nullptr, wifiPageDraw },
  { "Reinitialiser", REG_ACTION,   nullptr,                0, 0,   nullptr, 0, actionReset, nullptr },
};

const uint8_t NB_REGLAGES = sizeof(listeReglages) / sizeof(listeReglages[0]);