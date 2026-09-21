#include "Widgets.h"

lv_obj_t *creerLigne(lv_obj_t *parent, const char *nom) {
  lv_obj_t *ligne = lv_obj_create(parent);
  lv_obj_set_size(ligne, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(ligne, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(ligne, 0, 0);
  lv_obj_set_style_pad_all(ligne, 2, 0);
  lv_obj_set_scrollable(ligne, false);
  lv_obj_set_flex_flow(ligne, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ligne, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_label_set_text(lv_label_create(ligne), nom);
  return ligne;
}

lv_obj_t *creerBoutonPage(lv_obj_t *parent, const char *nom, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text_fmt(label, "%s  " LV_SYMBOL_RIGHT, nom);
  lv_obj_center(label);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

// ---------- Slider composé ----------
// Le label de valeur est rangé dans le user_data du slider
static void majValeur(lv_obj_t *slider) {
  lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(slider);
  int32_t v   = lv_slider_get_value(slider);
  int32_t max = lv_slider_get_max_value(slider);
  lv_label_set_text_fmt(label, "%d%%", (int)(v * 100 / max));
}

static void sliderValeurCb(lv_event_t *e) {
  majValeur((lv_obj_t *)lv_event_get_target(e));
}

// Au focus, on montre tout le bloc (libellé compris), pas seulement le slider
static void sliderFocusCb(lv_event_t *e) {
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_scroll_to_view(lv_obj_get_parent(slider), LV_ANIM_ON);
}

lv_obj_t *creerSlider(lv_obj_t *parent, const char *nom,
                      int32_t min, int32_t max, lv_event_cb_t cb) {
  // Bloc vertical transparent
  lv_obj_t *bloc = lv_obj_create(parent);
  lv_obj_set_size(bloc, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(bloc, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bloc, 0, 0);
  lv_obj_set_style_pad_all(bloc, 4, 0);        // Place pour le contour de focus
  lv_obj_set_style_pad_row(bloc, 10, 0);
  lv_obj_set_scrollable(bloc, false);
  lv_obj_set_flex_flow(bloc, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(bloc, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // En-tête : nom à gauche, valeur à droite
  lv_obj_t *entete = creerLigne(bloc, nom);
  lv_obj_t *valeur = lv_label_create(entete);

  // Le slider
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
  majValeur(slider);         // Un set_value par le code n'envoie pas d'événement
}