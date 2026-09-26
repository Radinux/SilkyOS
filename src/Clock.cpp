#include <Arduino.h>
#include <string.h>
#include "Clock.h"
#include "Storage.h"

// Fuseaux en heures entières, de UTC-12 à UTC+14
static const int FUSEAU_MIN = -12;
static const int NB_FUSEAUX = 27;

// ---------- Calendrier ----------
// Nombre de jours entre le 1er janvier 1970 et une date (algorithme de Howard Hinnant)
static int32_t joursDepuis1970(int y, int m, int d) {
  y -= (m <= 2);
  int32_t ere  = (y >= 0 ? y : y - 399) / 400;
  int32_t anEre = y - ere * 400;
  int32_t jourAn = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int32_t jourEre = anEre * 365 + anEre / 4 - anEre / 100 + jourAn;
  return ere * 146097 + jourEre - 719468;
}

// Jour (compté depuis 1970) du dernier dimanche d'un mois de 31 jours
static int32_t dernierDimanche(int annee, int mois) {
  int32_t j31 = joursDepuis1970(annee, mois, 31);
  int jourSemaine = (j31 + 4) % 7;          // Le 1/1/1970 était un jeudi ; 0 = dimanche
  return j31 - jourSemaine;
}

// Règle de l'UE : du dernier dimanche de mars au dernier dimanche d'octobre, à 1 h UTC.
// Le changement a lieu au MÊME instant dans toute l'UE : on raisonne directement en UTC.
static bool estHeureEteUE(time_t utc) {
  struct tm t;
  gmtime_r(&utc, &t);
  int annee = t.tm_year + 1900;

  time_t debut = (time_t)dernierDimanche(annee, 3)  * 86400 + 3600;
  time_t fin   = (time_t)dernierDimanche(annee, 10) * 86400 + 3600;
  return utc >= debut && utc < fin;
}

// ---------- API ----------
void clockDemarrerSynchro() {
  // L'heure système reste en UTC : fuseau et heure d'été sont appliqués à l'affichage
  configTime(0, 0, "pool.ntp.org", "time.google.com");
}

bool clockValide() {
  // Avant synchro, l'horloge démarre en 1970 : tout ce qui est après 2023 est une vraie heure
  return time(nullptr) > 1700000000;
}

bool clockHeureEte() {
  return reglages.heureEte && clockValide() && estHeureEteUE(time(nullptr));
}

bool clockGet(struct tm *out) {
  if (!clockValide()) return false;

  time_t t = time(nullptr) + (time_t)(FUSEAU_MIN + reglages.fuseau) * 3600;
  if (clockHeureEte()) t += 3600;          // +1 h l'été
  gmtime_r(&t, out);
  return true;
}

const char *clockFuseauOptions() {
  static char options[NB_FUSEAUX * 8] = "";
  if (options[0] == '\0') {
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