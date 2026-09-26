#include <Arduino.h>
#include "../Display.h"
#include "../Lvgl.h"
#include "../Storage.h"
#include "../Ui.h"
#include "UiInterne.h"

// Durées en secondes, dans le même ordre que uiVeilleOptions() (0 = jamais)
static const uint16_t DUREES[]   = { 0, 15, 30, 60, 120, 300 };
static const uint8_t  NB_DUREES  = sizeof(DUREES) / sizeof(DUREES[0]);
static bool           enVeille   = false;

const char *uiVeilleOptions() {
  return "Jamais\n15 s\n30 s\n1 min\n2 min\n5 min";
}

void veilleReveiller() {
  if (!enVeille) return;
  enVeille = false;
  lvglSetVeille(false);
  displaySetBrightness(reglages.luminosite);
}

// autorisee = false pendant une mise à jour ou une alerte : on reste (ou on se remet) allumé
void veilleMaj(bool autorisee) {
  if (enVeille) {
    if (lvglPopReveil() || !autorisee) veilleReveiller();
    return;
  }

  uint16_t duree = DUREES[reglages.veille < NB_DUREES ? reglages.veille : 0];
  if (!autorisee || duree == 0) return;

  if (lvglInactivite() > duree * 1000UL) {
    enVeille = true;
    lvglSetVeille(true);
    displayEteindre();
  }
}