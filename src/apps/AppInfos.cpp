#include <Arduino.h>
#include "../App.h"
#include "../Theme.h"
#include "../Widgets.h"

static lv_obj_t   *barreRam, *barrePsram, *labelRam, *labelPsram, *labelUptime;
static lv_timer_t *timer = nullptr;

static void majBarre(lv_obj_t *barre, lv_obj_t *label, uint32_t utilise, uint32_t total) {
  int pct = total ? (int)((uint64_t)utilise * 100 / total) : 0;
  lv_bar_set_value(barre, pct, LV_ANIM_ON);

  // Accent du thème, puis orange et rouge quand ça se remplit
  uint32_t c = pct > 75 ? COUL_ROUGE : pct > 50 ? COUL_ATTENTION : COUL_ACCENT;
  lv_obj_set_style_bg_color(barre, lv_color_hex(c), LV_PART_INDICATOR);

  lv_label_set_text_fmt(label, "%lu/%lu ko",
                        (unsigned long)(utilise / 1024), (unsigned long)(total / 1024));
}

static void majInfos(lv_timer_t *) {
  uint32_t ht = ESP.getHeapSize(),  hl = ESP.getFreeHeap();
  uint32_t pt = ESP.getPsramSize(), pl = ESP.getFreePsram();

  majBarre(barreRam,   labelRam,   ht - hl, ht);
  majBarre(barrePsram, labelPsram, pt - pl, pt);

  unsigned long s = millis() / 1000;
  lv_label_set_text_fmt(labelUptime, "%02lu:%02lu:%02lu", s / 3600, (s / 60) % 60, s % 60);
}

void infosCreate(lv_obj_t *contenu) {
  // --- Carte héros : la puce ---
  lv_obj_t *heros = creerCarte(contenu);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *puce = lv_label_create(heros);
  lv_label_set_text(puce, ESP.getChipModel());
  lv_obj_set_style_text_font(puce, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(puce, lv_color_hex(COUL_ACCENT), 0);

  lv_obj_t *details = lv_label_create(heros);
  lv_label_set_text_fmt(details, "%d coeurs - %lu MHz",
                        ESP.getChipCores(), (unsigned long)getCpuFrequencyMhz());
  lv_obj_set_style_text_color(details, lv_color_hex(COUL_TEXTE_2), 0);

  // --- Mémoire ---
  barreRam   = creerJauge(contenu, "RAM",   &labelRam);
  barrePsram = creerJauge(contenu, "PSRAM", &labelPsram);

  // --- Uptime ---
  labelUptime = creerInfo(contenu, "Uptime");
  lv_obj_set_style_text_font(labelUptime, &lv_font_montserrat_20, 0);

  majInfos(nullptr);
  timer = lv_timer_create(majInfos, 500, nullptr);
}

void infosExit() {
  if (timer) { lv_timer_delete(timer); timer = nullptr; }
}