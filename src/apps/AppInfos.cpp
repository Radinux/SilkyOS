#include <Arduino.h>
#include "../App.h"

static lv_obj_t   *barreRam, *barrePsram, *labelRam, *labelPsram, *labelUptime;
static lv_timer_t *timer = nullptr;

// Crée "Nom ........ valeur" puis une barre en dessous
static lv_obj_t *creerBarre(lv_obj_t *parent, const char *nom, lv_obj_t **labelValeur) {
  lv_obj_t *ligne = lv_obj_create(parent);
  lv_obj_set_size(ligne, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(ligne, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(ligne, 0, 0);
  lv_obj_set_style_pad_all(ligne, 0, 0);
  lv_obj_set_flex_flow(ligne, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ligne, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_label_set_text(lv_label_create(ligne), nom);
  *labelValeur = lv_label_create(ligne);

  lv_obj_t *barre = lv_bar_create(parent);
  lv_obj_set_width(barre, lv_pct(100));
  lv_bar_set_range(barre, 0, 100);
  return barre;
}

static void majBarre(lv_obj_t *barre, lv_obj_t *label, uint32_t utilise, uint32_t total) {
  int pct = total ? (int)((uint64_t)utilise * 100 / total) : 0;

  lv_bar_set_value(barre, pct, LV_ANIM_ON);    // Transition animée, gratuite 😏

  lv_color_t c = pct > 75 ? lv_palette_main(LV_PALETTE_RED)
               : pct > 50 ? lv_palette_main(LV_PALETTE_ORANGE)
               :            lv_palette_main(LV_PALETTE_GREEN);
  lv_obj_set_style_bg_color(barre, c, LV_PART_INDICATOR);

  lv_label_set_text_fmt(label, "%lu / %lu ko",
                        (unsigned long)(utilise / 1024), (unsigned long)(total / 1024));
}

// Appelé toutes les 500 ms par un timer LVGL
static void majInfos(lv_timer_t *) {
  uint32_t ht = ESP.getHeapSize(),  hl = ESP.getFreeHeap();
  uint32_t pt = ESP.getPsramSize(), pl = ESP.getFreePsram();

  majBarre(barreRam,   labelRam,   ht - hl, ht);
  majBarre(barrePsram, labelPsram, pt - pl, pt);

  unsigned long s = millis() / 1000;
  lv_label_set_text_fmt(labelUptime, "%02lu:%02lu:%02lu", s / 3600, (s / 60) % 60, s % 60);
}

void infosCreate(lv_obj_t *contenu) {
  lv_obj_set_style_pad_row(contenu, 6, 0);

  lv_label_set_text(lv_label_create(contenu), ESP.getChipModel());
  lv_label_set_text_fmt(lv_label_create(contenu), "%d coeurs - %lu MHz",
                        ESP.getChipCores(), (unsigned long)getCpuFrequencyMhz());

  barreRam   = creerBarre(contenu, "RAM",   &labelRam);
  barrePsram = creerBarre(contenu, "PSRAM", &labelPsram);

  lv_label_set_text(lv_label_create(contenu), "Uptime");
  labelUptime = lv_label_create(contenu);
  lv_obj_set_style_text_font(labelUptime, &lv_font_montserrat_28, 0);

  majInfos(nullptr);                                 // Remplissage immédiat
  timer = lv_timer_create(majInfos, 500, nullptr);   // Puis toutes les 500 ms
}

void infosExit() {
  // Indispensable : le timer mettrait à jour des labels supprimés → crash
  if (timer) { lv_timer_delete(timer); timer = nullptr; }
}