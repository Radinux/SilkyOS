#ifndef UI_H
#define UI_H

#include <lvgl.h>

// ---------- Pour main.cpp ----------
void uiInit();
void uiUpdate();

// ---------- Pour les apps ----------
// Ouvre une sous-page depuis une app. Moyen = retour à l'app, long = menu.
void uiOuvrirPage(const char *titre, void (*onCreate)(lv_obj_t *),
                  void (*onExit)());
void uiRecharger(); // Reconstruit l'écran courant
void uiAlerte(const char *titre,
              const char *texte); // Alerte plein écran bloquante
bool uiPaysage();                 // L'écran est-il plus large que haut ?
const char *uiVeilleOptions();    // "Jamais\n15 s\n..." pour un dropdown
bool uiEnVeille(); // En veille (écran éteint ou AOD) ? Pour ralentir la boucle

#endif