#include "Widgets.h"
#include "Theme.h"

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