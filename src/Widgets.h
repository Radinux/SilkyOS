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

// Carte "Libellé ......... [valeur]" avec liste de choix, valeur centrée
lv_obj_t *creerDropdown(lv_obj_t *parent, const char *nom,
                        const char *options, lv_event_cb_t cb);

// Change le choix d'un dropdown créé par creerDropdown() (met aussi le texte à jour)
void dropdownSetValeur(lv_obj_t *dd, uint32_t index);

// Rangée transparente pour poser des éléments côte à côte (boutons, rouleaux...)
lv_obj_t *creerRangeeVide(lv_obj_t *parent);

// Bouton-carte au texte centré, qui se partage la largeur d'une creerRangeeVide()
lv_obj_t *creerBoutonAction(lv_obj_t *parent, const char *texte,
                            uint32_t couleurTexte, lv_event_cb_t cb);
void      boutonSetTexte(lv_obj_t *btn, const char *texte, uint32_t couleur);

// Rend une carte atteignable par l'encodeur (pour la faire défiler), sans surbrillance
void rendreConsultable(lv_obj_t *obj);

// Paysage : le contenu devient deux colonnes côte à côte (gauche ~55 %, droite ~45 %).
// Portrait : gauche et droite valent simplement le contenu, rien ne change.
void creerColonnes(lv_obj_t *contenu, lv_obj_t **gauche, lv_obj_t **droite);

#endif