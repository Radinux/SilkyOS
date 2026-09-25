#ifndef WIDGETS_H
#define WIDGETS_H

#include <lvgl.h>

// Carte verticale vide, à remplir
lv_obj_t *creerCarte(lv_obj_t *parent);

// Carte "Libellé ......... [widget]" : ajoute ton widget dedans
lv_obj_t *creerLigne(lv_obj_t *parent, const char *nom);

// Carte cliquable "Nom .......... >" qui ouvre une sous-page
lv_obj_t *creerBoutonPage(lv_obj_t *parent, const char *nom, lv_event_cb_t cb);

// Carte "Libellé ... valeur%" + slider
lv_obj_t *creerSlider(lv_obj_t *parent, const char *nom,
                      int32_t min, int32_t max, lv_event_cb_t cb);
void      sliderSetValeur(lv_obj_t *slider, int32_t valeur);

// Carte "petit titre gris / valeur" : renvoie le label de valeur
lv_obj_t *creerInfo(lv_obj_t *parent, const char *titre);

// Carte "Nom ........ valeur" + barre : renvoie la barre, et le label via labelValeur
lv_obj_t *creerJauge(lv_obj_t *parent, const char *nom, lv_obj_t **labelValeur);

#endif