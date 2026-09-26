#include <Arduino.h>
#include <lvgl.h>
#include "../Battery.h"
#include "../Button.h"
#include "../Clock.h"
#include "../Lvgl.h"
#include "../Theme.h"
#include "UiInterne.h"

static lv_obj_t *labelHeure, *labelBatterie, *barreAppui;

// ---------- Barre d'état (calque supérieur) ----------
static lv_obj_t *creerLabelEtat(lv_align_t align, int32_t x) {
  lv_obj_t *l = lv_label_create(lv_layer_top());
  lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(COUL_TEXTE_2), 0);
  lv_obj_align(l, align, x, 4);
  lv_obj_set_hidden(l, true);                  // Visibles à partir du launcher
  return l;
}

// Ne redessine que quand la minute change
static void majHeure() {
  static int derniereMinute = -2;
  struct tm t;
  int minute = clockGet(&t) ? t.tm_hour * 60 + t.tm_min : -1;
  if (minute == derniereMinute) return;
  derniereMinute = minute;

  if (minute < 0) lv_label_set_text(labelHeure, "--:--");
  else            lv_label_set_text_fmt(labelHeure, "%02d:%02d", t.tm_hour, t.tm_min);
}

// Ne redessine que quand l'affichage change
static void majBatterie() {
  static int dernierAffiche = -2;                 // -1 = USB
  int affiche = batteryOnUsb() ? -1 : batteryPercent();
  if (affiche == dernierAffiche) return;
  dernierAffiche = affiche;

  if (affiche < 0) {                              // Sur USB, le niveau n'est pas lisible
    lv_label_set_text(labelBatterie, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(labelBatterie, lv_color_hex(COUL_VERT), 0);
    return;
  }

  static const char *symboles[] = {
    LV_SYMBOL_BATTERY_EMPTY, LV_SYMBOL_BATTERY_1, LV_SYMBOL_BATTERY_2,
    LV_SYMBOL_BATTERY_3, LV_SYMBOL_BATTERY_FULL,
  };
  int niveau = affiche >= 90 ? 4 : affiche >= 65 ? 3 : affiche >= 40 ? 2 : affiche >= 15 ? 1 : 0;

  lv_label_set_text_fmt(labelBatterie, "%d%% %s", affiche, symboles[niveau]);
  lv_obj_set_style_text_color(labelBatterie,
      lv_color_hex(niveau == 0 ? COUL_ROUGE : COUL_TEXTE_2), 0);
}

// ---------- Barre d'appui (bas de l'écran) ----------
static void creerBarreAppui() {
  barreAppui = lv_bar_create(lv_layer_top());
  lv_obj_set_size(barreAppui, lv_pct(100), 6);
  lv_obj_align(barreAppui, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, LV_PART_INDICATOR);
  lv_bar_set_range(barreAppui, 0, LONG_PRESS_INTERVAL);
  lv_group_remove_obj(barreAppui);

  // Repère blanc au seuil "moyen"
  lv_obj_t *repere = lv_obj_create(barreAppui);
  lv_obj_set_size(repere, 2, lv_pct(100));
  lv_obj_set_x(repere, lv_pct(SHORT_PRESS_INTERVAL * 100 / LONG_PRESS_INTERVAL));
  lv_obj_set_style_bg_color(repere, lv_color_white(), 0);
  lv_obj_set_style_border_width(repere, 0, 0);
  lv_obj_set_style_radius(repere, 0, 0);
  lv_obj_set_style_pad_all(repere, 0, 0);
  lv_obj_set_scrollable(repere, false);

  lv_obj_set_hidden(barreAppui, true);
}

static void majAppui() {
  static bool visible = false;

  if (!lvglBoutonPresse()) {
    if (visible) { lv_obj_set_hidden(barreAppui, true); visible = false; }
    return;
  }

  uint32_t duree = millis() - lvglDebutAppui();

  lv_color_t c = duree < SHORT_PRESS_INTERVAL ? lv_palette_main(LV_PALETTE_GREEN)
               : duree < LONG_PRESS_INTERVAL  ? lv_palette_main(LV_PALETTE_ORANGE)
               :                                lv_palette_main(LV_PALETTE_RED);
  lv_obj_set_style_bg_color(barreAppui, c, LV_PART_INDICATOR);

  if (duree > LONG_PRESS_INTERVAL) duree = LONG_PRESS_INTERVAL;
  lv_bar_set_value(barreAppui, duree, LV_ANIM_OFF);

  if (!visible) { lv_obj_set_hidden(barreAppui, false); visible = true; }
}

// ---------- API interne ----------
void barresCreer() {
  labelHeure    = creerLabelEtat(LV_ALIGN_TOP_LEFT,   8);
  labelBatterie = creerLabelEtat(LV_ALIGN_TOP_RIGHT, -8);
  creerBarreAppui();
}

void barresMontrer() {
  lv_obj_set_hidden(labelHeure,    false);
  lv_obj_set_hidden(labelBatterie, false);
}

void barresMaj() {
  majHeure();
  majBatterie();
  majAppui();
}