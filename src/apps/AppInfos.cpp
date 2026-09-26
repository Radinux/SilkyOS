#include <Arduino.h>
#include <esp_system.h>
#include "../App.h"
#include "../Theme.h"
#include "../Widgets.h"
#include "../Battery.h"
#include "../Diag.h"

static lv_obj_t   *barreBatt, *labelBatt, *labelTension;
static lv_obj_t   *barreRam, *barrePsram, *labelRam, *labelPsram, *labelUptime;
static lv_timer_t *timer = nullptr;

static void majBarre(lv_obj_t *barre, lv_obj_t *label, uint32_t utilise, uint32_t total) {
  int pct = total ? (int)((uint64_t)utilise * 100 / total) : 0;
  lv_bar_set_value(barre, pct, LV_ANIM_ON);

  uint32_t c = pct > 75 ? COUL_ROUGE : pct > 50 ? COUL_ATTENTION : COUL_ACCENT;
  lv_obj_set_style_bg_color(barre, lv_color_hex(c), LV_PART_INDICATOR);

  lv_label_set_text_fmt(label, "%lu/%lu ko",
                        (unsigned long)(utilise / 1024), (unsigned long)(total / 1024));
}

static void majBatterie() {
  // LVGL n'affiche pas les float par défaut : on formate les volts à la main
  int mv = (int)(batteryVolts() * 1000);
  lv_label_set_text_fmt(labelTension, "%d.%02d V", mv / 1000, (mv % 1000) / 10);

  if (batteryOnUsb()) {
    lv_bar_set_value(barreBatt, 100, LV_ANIM_ON);
    lv_obj_set_style_bg_color(barreBatt, lv_color_hex(COUL_VERT), LV_PART_INDICATOR);
    lv_label_set_text(labelBatt, "USB");
  } else {
    int p = batteryPercent();
    lv_bar_set_value(barreBatt, p, LV_ANIM_ON);
    uint32_t c = p < 15 ? COUL_ROUGE : p < 30 ? COUL_ATTENTION : COUL_VERT;
    lv_obj_set_style_bg_color(barreBatt, lv_color_hex(c), LV_PART_INDICATOR);
    lv_label_set_text_fmt(labelBatt, "%d%%", p);
  }
}

static void majInfos(lv_timer_t *) {
  majBatterie();

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

  // --- Batterie ---
  barreBatt    = creerJauge(contenu, "Batterie", &labelBatt);
  labelTension = creerInfo(contenu, "Tension");

  // --- Mémoire ---
  barreRam   = creerJauge(contenu, "RAM",   &labelRam);
  barrePsram = creerJauge(contenu, "PSRAM", &labelPsram);

  // --- Uptime ---
  labelUptime = creerInfo(contenu, "Uptime");
  lv_obj_set_style_text_font(labelUptime, &lv_font_montserrat_20, 0);

  // --- Diagnostic : pourquoi la carte a-t-elle démarré ? ---
  lv_obj_t *labelReset = creerInfo(contenu, "Dernier reset");
  lv_label_set_text(labelReset, diagRaisonReset());

  esp_reset_reason_t r = esp_reset_reason();
  bool anormal = (r == ESP_RST_PANIC   || r == ESP_RST_BROWNOUT ||
                  r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT);
  if (anormal) lv_obj_set_style_text_color(labelReset, lv_color_hex(COUL_ROUGE), 0);

  majInfos(nullptr);
  timer = lv_timer_create(majInfos, 500, nullptr);
}

void infosExit() {
  if (timer) { lv_timer_delete(timer); timer = nullptr; }
}