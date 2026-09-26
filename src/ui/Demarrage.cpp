#include <lvgl.h>
#include "../Config.h"
#include "../Theme.h"
#include "../Ui.h"
#include "../Widgets.h"
#include "UiInterne.h"

static void finCb(lv_timer_t *) {
  uiAfficherMenu(LV_SCR_LOAD_ANIM_FADE_IN);       // Supprime l'écran de boot au passage
}

static void creerTexteLogo(lv_obj_t *parent, const char *texte, uint32_t couleur) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, texte);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(couleur), 0);
}

void demarrageAfficher() {
  lv_obj_t *ecran = uiCentrer(lv_obj_create(nullptr));
  lv_obj_set_style_bg_color(ecran, lv_color_hex(COUL_FOND), 0);

  // Logo : "Silky" en blanc + "OS" en couleur d'accent, collés
  lv_obj_t *logo = creerRangeeVide(ecran);
  lv_obj_set_style_pad_column(logo, 0, 0);
  creerTexteLogo(logo, "Silky", COUL_TEXTE);
  creerTexteLogo(logo, "OS",    COUL_ACCENT);
  lv_obj_fade_in(logo, 600, 0);

  lv_obj_t *version = lv_label_create(ecran);
  lv_label_set_text(version, "v" SILKY_VERSION);
  lv_obj_set_style_text_color(version, lv_color_hex(COUL_TEXTE_2), 0);

  if (uiPaysage()) {
    // Paysage : peu de hauteur, une fine barre qui se remplit pendant le démarrage
    lv_obj_t *barre = lv_bar_create(ecran);
    lv_obj_set_size(barre, lv_pct(50), 4);
    lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_CARTE_FOCUS), 0);
    lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_anim_duration(barre, 1600, 0);
    lv_bar_set_value(barre, 100, LV_ANIM_ON);
  } else {
    // Portrait : un spinner fin
    lv_obj_t *spinner = lv_spinner_create(ecran);
    lv_obj_set_size(spinner, 30, 30);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COUL_CARTE_FOCUS), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);
    lv_group_remove_obj(spinner);
  }

  lv_screen_load_anim(ecran, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);

  lv_timer_t *t = lv_timer_create(finCb, 1800, nullptr);    // Timer "one-shot"
  lv_timer_set_repeat_count(t, 1);
}