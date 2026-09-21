#ifndef LVGL_GLUE_H
#define LVGL_GLUE_H

#include <stdint.h>

void     lvglInit();
bool     lvglPopRetour();     // Vrai une fois après un appui moyen
bool     lvglPopMenu();       // Vrai une fois après un appui long
bool     lvglBoutonPresse();  // Pour la barre d'appui (étape 5)
uint32_t lvglDebutAppui();

#endif
