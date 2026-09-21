#ifndef UI_H
#define UI_H

#include <lvgl.h>

void uiInit();
void uiUpdate();

// Ouvre une sous-page depuis une app. Moyen = retour à l'app, long = menu.
void uiOuvrirPage(const char *titre, void (*onCreate)(lv_obj_t *), void (*onExit)());

#endif