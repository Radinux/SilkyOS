#ifndef WIDGETS_H
#define WIDGETS_H

#include <lvgl.h>

// Ligne "Libellé ......... [widget]" : ajoute ton widget dedans
lv_obj_t *creerLigne(lv_obj_t *parent, const char *nom);

// Bouton pleine largeur "Nom  >" qui ouvre une sous-page
lv_obj_t *creerBoutonPage(lv_obj_t *parent, const char *nom, lv_event_cb_t cb);

// Bloc "Libellé ... valeur%" + slider, avec scroll du bloc entier au focus
lv_obj_t *creerSlider(lv_obj_t *parent, const char *nom,
                      int32_t min, int32_t max, lv_event_cb_t cb);

// Change la valeur d'un slider créé par creerSlider() (met aussi le % à jour)
void sliderSetValeur(lv_obj_t *slider, int32_t valeur);

#endif