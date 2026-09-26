#include <Arduino.h>
#include <string.h>
#include "Clock.h"
#include "Storage.h"

// Fuseaux en heures entières, de UTC-12 à UTC+14
static const int FUSEAU_MIN = -12;
static const int NB_FUSEAUX = 27;

void clockDemarrerSynchro() {
  // L'heure système reste en UTC : le fuseau est appliqué à l'affichage.
  // Changer de fuseau est donc instantané, sans nouvelle synchro.
  configTime(0, 0, "pool.ntp.org", "time.google.com");
}

bool clockValide() {
  // Avant synchro, l'horloge démarre en 1970 : tout ce qui est après 2023 est une vraie heure
  return time(nullptr) > 1700000000;
}

bool clockGet(struct tm *out) {
  if (!clockValide()) return false;
  time_t t = time(nullptr) + (time_t)(FUSEAU_MIN + reglages.fuseau) * 3600;
  gmtime_r(&t, out);          // gmtime : on a déjà appliqué le décalage nous-mêmes
  return true;
}

const char *clockFuseauOptions() {
  static char options[NB_FUSEAUX * 8] = "";
  if (options[0] == '\0') {   // Construit une seule fois
    for (int i = 0; i < NB_FUSEAUX; i++) {
      char item[8];
      int h = FUSEAU_MIN + i;
      snprintf(item, sizeof(item), h < 0 ? "UTC%d" : "UTC+%d", h);
      strcat(options, item);
      if (i < NB_FUSEAUX - 1) strcat(options, "\n");
    }
  }
  return options;
}