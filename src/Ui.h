#ifndef UI_H
#define UI_H

#include <lvgl.h>

void uiInit();
void uiUpdate();
void uiRecharger();   // Reconstruit l'écran courant au prochain tour (changement de thème...)

const char *uiVeilleOptions();   // "Jamais\n15 s\n..." pour le dropdown des Réglages

// Ouvre une sous-page depuis une app. Moyen = retour à l'app, long = menu.
void uiOuvrirPage(const char *titre, void (*onCreate)(lv_obj_t *), void (*onExit)());

#endif