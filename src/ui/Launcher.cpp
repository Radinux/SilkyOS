#include <lvgl.h>
#include "../App.h"
#include "../Theme.h"
#include "UiInterne.h"

static void clicCb(lv_event_t *e) {
  uiOuvrirApp((int8_t)(intptr_t)lv_event_get_user_data(e), uiAnimEntree());
}

// Une app = une carte "[pastille] Nom ........ >"
static lv_obj_t *creerEntree(lv_obj_t *parent, uint8_t i) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
  themeCarte(btn);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(btn, 10, 0);

  // Pastille colorée avec l'icône
  lv_obj_t *pastille = lv_obj_create(btn);
  lv_obj_set_size(pastille, 30, 30);
  lv_obj_set_style_radius(pastille, 8, 0);
  lv_obj_set_style_bg_color(pastille, lv_color_hex(apps[i].couleur), 0);
  lv_obj_set_style_border_width(pastille, 0, 0);
  lv_obj_set_style_pad_all(pastille, 0, 0);
  lv_obj_set_scrollable(pastille, false);

  lv_obj_t *icone = lv_label_create(pastille);
  lv_label_set_text(icone, apps[i].icone);
  lv_obj_set_style_text_color(icone, lv_color_white(), 0);
  lv_obj_center(icone);

  // Nom : prend toute la place, ce qui pousse le chevron à droite
  lv_obj_t *nom = lv_label_create(btn);
  lv_label_set_text(nom, apps[i].nom);
  lv_obj_set_flex_grow(nom, 1);

  lv_obj_t *chevron = lv_label_create(btn);
  lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron, lv_color_hex(COUL_TEXTE_2), 0);

  lv_obj_add_event_cb(btn, clicCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  return btn;
}

void launcherAfficher(int8_t selection, lv_screen_load_anim_t anim) {
  lv_obj_t *ecran;
  lv_obj_t *contenu = uiCreerEcran("SilkyOS", &ecran);

  for (uint8_t i = 0; i < NB_APPS; i++) {
    lv_obj_t *btn = creerEntree(contenu, i);
    if (i == selection) lv_group_focus_obj(btn);   // Retour sur l'app qu'on vient de quitter
  }

  uiChargerEcran(ecran, contenu, anim);
}