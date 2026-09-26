#include "Widgets.h"
#include "Theme.h"
#include "Ui.h"
// ---------- Briques de base ----------
lv_obj_t *creerCarte(lv_obj_t *parent) {
  lv_obj_t *carte = lv_obj_create(parent);
  lv_obj_set_size(carte, lv_pct(100), LV_SIZE_CONTENT);
  themeCarte(carte);
  lv_obj_set_scrollable(carte, false);
  lv_obj_set_flex_flow(carte, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(carte, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(carte, 6, 0);
  return carte;
}

// Rangée "Libellé ......... [widget]", en carte ou transparente.
// Rappel : un style local l'emporte TOUJOURS sur un style ajouté.
static lv_obj_t *creerRangee(lv_obj_t *parent, const char *nom, bool carte) {
  lv_obj_t *ligne = lv_obj_create(parent);
  lv_obj_set_size(ligne, lv_pct(100), LV_SIZE_CONTENT);

  if (carte) {
    themeCarte(ligne);
  } else {
    lv_obj_set_style_bg_opa(ligne, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ligne, 0, 0);
    lv_obj_set_style_pad_all(ligne, 0, 0);
  }

  lv_obj_set_scrollable(ligne, false);
  lv_obj_set_flex_flow(ligne, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ligne, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(ligne, 8, 0);     // Écart minimum libellé ↔ widget

  lv_obj_t *label = lv_label_create(ligne);
  lv_label_set_text(label, nom);
  lv_obj_set_style_text_color(label, lv_color_hex(COUL_TEXTE), 0);
  return ligne;
}

lv_obj_t *creerLigne(lv_obj_t *parent, const char *nom) {
  return creerRangee(parent, nom, true);
}

lv_obj_t *creerBoutonPage(lv_obj_t *parent, const char *nom, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
  themeCarte(btn);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_label_set_text(lv_label_create(btn), nom);

  lv_obj_t *chevron = lv_label_create(btn);
  lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron, lv_color_hex(COUL_TEXTE_2), 0);

  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

// ---------- Slider composé ----------
static void majValeur(lv_obj_t *slider) {
  lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(slider);
  int32_t v   = lv_slider_get_value(slider);
  int32_t max = lv_slider_get_max_value(slider);
  lv_label_set_text_fmt(label, "%d%%", (int)(v * 100 / max));
}

static void sliderValeurCb(lv_event_t *e) {
  majValeur((lv_obj_t *)lv_event_get_target(e));
}

static void sliderFocusCb(lv_event_t *e) {
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_scroll_to_view(lv_obj_get_parent(slider), LV_ANIM_ON);
}

lv_obj_t *creerSlider(lv_obj_t *parent, const char *nom,
                      int32_t min, int32_t max, lv_event_cb_t cb) {
  lv_obj_t *bloc = creerCarte(parent);
  lv_obj_set_style_pad_row(bloc, 12, 0);
  lv_obj_set_flex_align(bloc, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *entete = creerRangee(bloc, nom, false);
  lv_obj_t *valeur = lv_label_create(entete);
  lv_obj_set_style_text_color(valeur, lv_color_hex(COUL_TEXTE_2), 0);

  lv_obj_t *slider = lv_slider_create(bloc);
  lv_obj_set_width(slider, lv_pct(90));
  lv_slider_set_range(slider, min, max);
  lv_obj_set_user_data(slider, valeur);

  lv_obj_add_event_cb(slider, sliderValeurCb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(slider, sliderFocusCb,  LV_EVENT_FOCUSED,       nullptr);
  if (cb) lv_obj_add_event_cb(slider, cb, LV_EVENT_VALUE_CHANGED, nullptr);

  majValeur(slider);
  return slider;
}

void sliderSetValeur(lv_obj_t *slider, int32_t valeur) {
  lv_slider_set_value(slider, valeur, LV_ANIM_OFF);
  majValeur(slider);
}

// ---------- Affichage d'informations ----------
lv_obj_t *creerInfo(lv_obj_t *parent, const char *titre) {
  lv_obj_t *carte = creerCarte(parent);
  lv_obj_set_style_pad_row(carte, 2, 0);

  lv_obj_t *t = lv_label_create(carte);
  lv_label_set_text(t, titre);
  lv_obj_set_style_text_color(t, lv_color_hex(COUL_TEXTE_2), 0);

  lv_obj_t *valeur = lv_label_create(carte);
  lv_label_set_text(valeur, "-");
  lv_obj_set_style_text_color(valeur, lv_color_hex(COUL_TEXTE), 0);
  return valeur;
}

lv_obj_t *creerJauge(lv_obj_t *parent, const char *nom, lv_obj_t **labelValeur) {
  lv_obj_t *carte = creerCarte(parent);
  lv_obj_set_style_pad_row(carte, 8, 0);

  lv_obj_t *entete = creerRangee(carte, nom, false);
  *labelValeur = lv_label_create(entete);
  lv_obj_set_style_text_color(*labelValeur, lv_color_hex(COUL_TEXTE_2), 0);

  lv_obj_t *barre = lv_bar_create(carte);
  lv_obj_set_size(barre, lv_pct(100), 8);
  lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_CARTE_FOCUS), 0);
  lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);
  return barre;
}

// ---------- Dropdown composé ----------
// LVGL place lui-même le texte d'un dropdown et ignore text_align.
// On masque donc son texte et on pose notre propre label, centré.
static void majDropdown(lv_obj_t *dd) {
  lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(dd);
  char texte[32];
  lv_dropdown_get_selected_str(dd, texte, sizeof(texte));
  lv_label_set_text(label, texte);
}

static void dropdownValeurCb(lv_event_t *e) {
  majDropdown((lv_obj_t *)lv_event_get_target(e));
}

lv_obj_t *creerDropdown(lv_obj_t *parent, const char *nom,
                        const char *options, lv_event_cb_t cb) {
  lv_obj_t *ligne = creerLigne(parent, nom);
  lv_obj_t *dd = lv_dropdown_create(ligne);
  lv_dropdown_set_options(dd, options);

  // Style iOS : pas de flèche, pilule discrète
  lv_dropdown_set_symbol(dd, nullptr);
  lv_dropdown_set_text(dd, "");                 // Son texte à lui : vide
  lv_obj_set_width(dd, 60);
  lv_obj_set_scrollable(dd, false);
  lv_obj_set_style_bg_color(dd, lv_color_hex(COUL_CARTE_FOCUS), 0);
  lv_obj_set_style_border_width(dd, 0, 0);
  lv_obj_set_style_shadow_width(dd, 0, 0);
  lv_obj_set_style_radius(dd, 8, 0);
  lv_obj_set_style_pad_hor(dd, 6, 0);
  lv_obj_set_style_pad_ver(dd, 4, 0);

  // Notre label, parfaitement centré, en couleur d'accent
  lv_obj_t *valeur = lv_label_create(dd);
  lv_obj_set_style_text_color(valeur, lv_color_hex(COUL_ACCENT), 0);
  lv_obj_center(valeur);
  lv_obj_set_user_data(dd, valeur);

  lv_obj_add_event_cb(dd, dropdownValeurCb, LV_EVENT_VALUE_CHANGED, nullptr);
  if (cb) lv_obj_add_event_cb(dd, cb, LV_EVENT_VALUE_CHANGED, nullptr);

  majDropdown(dd);
  return dd;
}

void dropdownSetValeur(lv_obj_t *dd, uint32_t index) {
  lv_dropdown_set_selected(dd, index);
  majDropdown(dd);           // Comme pour le slider : un changement par le code n'envoie pas d'événement
}

// ---------- Rangées et boutons d'action ----------
lv_obj_t *creerRangeeVide(lv_obj_t *parent) {
  lv_obj_t *r = lv_obj_create(parent);
  lv_obj_set_size(r, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(r, 0, 0);
  lv_obj_set_style_pad_all(r, 4, 0);
  lv_obj_set_style_pad_column(r, 8, 0);
  lv_obj_set_scrollable(r, false);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  return r;
}

void boutonSetTexte(lv_obj_t *btn, const char *texte, uint32_t couleur) {
  lv_obj_t *label = lv_obj_get_child(btn, 0);     // Le label est le premier enfant
  lv_label_set_text(label, texte);
  lv_obj_set_style_text_color(label, lv_color_hex(couleur), 0);
}

lv_obj_t *creerBoutonAction(lv_obj_t *parent, const char *texte,
                            uint32_t couleurTexte, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_height(btn, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(btn, 1);                   // Largeur partagée avec ses voisins
  themeCarte(btn);
  lv_obj_set_style_pad_hor(btn, 4, 0);            // Local : gagne sur la marge de la carte

  lv_obj_t *label = lv_label_create(btn);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_center(label);

  boutonSetTexte(btn, texte, couleurTexte);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

// ---------- Cartes consultables ----------
void rendreConsultable(lv_obj_t *obj) {
  lv_group_add_obj(lv_group_get_default(), obj);          // Atteignable par l'encodeur
  lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);      // ...et on défile jusqu'à elle

  // Sans surbrillance : bordure gardée mais transparente, fond inchangé
  const lv_style_selector_t etats[] = { LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY };
  for (lv_style_selector_t s : etats) {
    lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, s);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COUL_CARTE), s);
  }
}

// ---------- Mise en page paysage ----------
static lv_obj_t *creerColonne(lv_obj_t *parent, uint8_t poids) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_set_height(c, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(c, poids);              // La largeur se répartit selon les poids
  lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(c, 0, 0);
  lv_obj_set_style_pad_all(c, 0, 0);
  lv_obj_set_style_pad_row(c, 8, 0);
  lv_obj_set_scrollable(c, false);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  return c;
}

void creerColonnes(lv_obj_t *contenu, lv_obj_t **gauche, lv_obj_t **droite) {
  if (!uiPaysage()) {
    *gauche = *droite = contenu;               // Portrait : une seule colonne, comme avant
    return;
  }

  lv_obj_set_flex_flow(contenu, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(contenu, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_column(contenu, 6, 0);

  *gauche = creerColonne(contenu, 11);         // 11 / 20 ≈ 55 %
  *droite = creerColonne(contenu, 9);          //  9 / 20 ≈ 45 %
}