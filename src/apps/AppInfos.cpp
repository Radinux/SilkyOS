#include <Arduino.h>
#include <esp_system.h>
#include <esp_ota_ops.h>   // Partition du firmware en cours
#include <nvs.h>           // Statistiques de la NVS
#include "../App.h"
#include "../Theme.h"
#include "../Widgets.h"
#include "../Battery.h"
#include "../Diag.h"

static lv_obj_t   *barreBatt, *labelBatt, *labelTension;
static lv_obj_t   *barreRam, *barrePsram, *labelRam, *labelPsram, *labelUptime;
static lv_timer_t *timer = nullptr;

// ---------- Jauges ----------
// Valeur, couleur (accent → orange → rouge quand ça se remplit), texte en Ko
static void majBarre(lv_obj_t *barre, lv_obj_t *label, uint32_t utilise, uint32_t total) {
  int pct = total ? (int)((uint64_t)utilise * 100 / total) : 0;
  lv_bar_set_value(barre, pct, LV_ANIM_ON);

  uint32_t c = pct > 75 ? COUL_ROUGE : pct > 50 ? COUL_ATTENTION : COUL_ACCENT;
  lv_obj_set_style_bg_color(barre, lv_color_hex(c), LV_PART_INDICATOR);

  lv_label_set_text_fmt(label, "%lu/%lu ko",
                        (unsigned long)(utilise / 1024), (unsigned long)(total / 1024));
}

// Même jauge, mais le texte en Mo avec une décimale (le printf de LVGL n'affiche pas les float)
static void majBarreMo(lv_obj_t *barre, lv_obj_t *label, uint32_t utilise, uint32_t total) {
  majBarre(barre, label, utilise, total);            // Valeur et couleur
  uint32_t u = utilise / 104858;                     // En dixièmes de Mo
  uint32_t t = total   / 104858;
  lv_label_set_text_fmt(label, "%lu.%lu/%lu.%lu Mo",
                        (unsigned long)(u / 10), (unsigned long)(u % 10),
                        (unsigned long)(t / 10), (unsigned long)(t % 10));
}

// ---------- Mises à jour périodiques ----------
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

  majBarre(barreRam,     labelRam,   ht - hl, ht);
  majBarreMo(barrePsram, labelPsram, pt - pl, pt);

  unsigned long s = millis() / 1000;
  lv_label_set_text_fmt(labelUptime, "%02lu:%02lu:%02lu", s / 3600, (s / 60) % 60, s % 60);
}

// ---------- API de la page ----------
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

  lv_obj_t *memoires = lv_label_create(heros);
  // Arrondi au Mo le plus proche (la puce annonce 8189 Ko de PSRAM, pas 8192)
  lv_label_set_text_fmt(memoires, "Flash %lu Mo - PSRAM %lu Mo",
                        (unsigned long)((ESP.getFlashChipSize() + (1 << 19)) >> 20),
                        (unsigned long)((ESP.getPsramSize()     + (1 << 19)) >> 20));
  lv_obj_set_width(memoires, lv_pct(100));                          // Passe à la ligne si besoin...
  lv_obj_set_style_text_align(memoires, LV_TEXT_ALIGN_CENTER, 0);   // ...en restant centré
  lv_obj_set_style_text_font(memoires, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(memoires, lv_color_hex(COUL_TEXTE_2), 0);

  // --- Batterie ---
  barreBatt    = creerJauge(contenu, "Batterie", &labelBatt);
  labelTension = creerInfo(contenu, "Tension");

  // --- Mémoire vive ---
  barreRam   = creerJauge(contenu, "RAM",   &labelRam);
  barrePsram = creerJauge(contenu, "PSRAM", &labelPsram);

  // --- Stockage : lu UNE seule fois, getSketchSize() relit tout le firmware en flash ! ---
  lv_obj_t *labelFirmware;
  lv_obj_t *barreFirmware = creerJauge(contenu, "Firmware", &labelFirmware);
  const esp_partition_t *emplacement = esp_ota_get_running_partition();
  majBarreMo(barreFirmware, labelFirmware, ESP.getSketchSize(), emplacement->size);

  nvs_stats_t stats;
  if (nvs_get_stats(nullptr, &stats) == ESP_OK) {
    lv_obj_t *labelNvs;
    lv_obj_t *barreNvs = creerJauge(contenu, "NVS", &labelNvs);
    majBarre(barreNvs, labelNvs, stats.used_entries, stats.total_entries);
    lv_label_set_text_fmt(labelNvs, "%u/%u", (unsigned)stats.used_entries, (unsigned)stats.total_entries);
  }

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