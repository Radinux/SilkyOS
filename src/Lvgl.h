#ifndef LVGL_GLUE_H
#define LVGL_GLUE_H

#include <stdint.h>

void     lvglInit();
void     lvglSetRotation(uint8_t rotation);   // Tourne l'écran et prévient LVGL
bool     lvglPopRetour();                     // Vrai une fois après un appui moyen
bool     lvglPopMenu();                       // Vrai une fois après un appui long
int32_t  lvglPopScroll();                     // Crans de défilement (pages sans widget)
bool     lvglBoutonPresse();                  // Pour la barre d'appui
uint32_t lvglDebutAppui();

// Veille
uint32_t lvglInactivite();                    // ms depuis la dernière action de l'utilisateur
void     lvglSetVeille(bool enVeille);        // En veille, les entrées ne servent qu'à réveiller
bool     lvglPopReveil();                     // Vrai une fois si l'utilisateur a réveillé l'écran

#endif