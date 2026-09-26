#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>

void        clockDemarrerSynchro();    // Lance la synchro NTP (appelé quand le WiFi Box est connecté)
bool        clockValide();             // L'heure a-t-elle été synchronisée ?
bool        clockGet(struct tm *out);  // Heure locale (fuseau appliqué). false si pas encore synchro
const char *clockFuseauOptions();      // "UTC-12\n...\nUTC+14" pour un dropdown
bool clockHeureEte();            // L'heure d'été est-elle appliquée en ce moment ?

#endif