#ifndef LVGL_GLUE_H
#define LVGL_GLUE_H

#include <stdint.h>

void     lvglInit();
void     lvglSetRotation(uint8_t rotation);   // Tourne l'écran et prévient LVGL
bool     lvglPopRetour();                     // Vrai une fois après un appui moyen
bool     lvglPopMenu();                       // Vrai une fois après un appui long
bool     lvglBoutonPresse();                  // Pour la barre d'appui
uint32_t lvglDebutAppui();

#endif