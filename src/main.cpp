#include <Arduino.h>
#include <lvgl.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Storage.h"
#include "Network.h"
#include "Lvgl.h"

// Appelé par LVGL quand un bouton est cliqué
static void clicCb(lv_event_t *e) {
  Serial.printf("Clic sur %s\n", (const char *)lv_event_get_user_data(e));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ATS-OS boot (LVGL) ===");

  storageInit();
  displayInit();
  encoderInit();
  netInit();
  lvglInit();

  // --- Test : trois boutons colorés empilés ---
  lv_obj_t *ecran = lv_screen_active();
  lv_obj_set_flex_flow(ecran, LV_FLEX_FLOW_COLUMN);               // Empile verticalement
  lv_obj_set_flex_align(ecran, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  static const char *noms[] = { "Rouge", "Vert", "Bleu" };
  lv_color_t couleurs[] = {
    lv_palette_main(LV_PALETTE_RED),
    lv_palette_main(LV_PALETTE_GREEN),
    lv_palette_main(LV_PALETTE_BLUE),
  };

  for (int i = 0; i < 3; i++) {
    lv_obj_t *btn = lv_button_create(ecran);
    lv_obj_set_width(btn, lv_pct(80));                            // 80 % de la largeur
    lv_obj_set_style_bg_color(btn, couleurs[i], 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, noms[i]);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, clicCb, LV_EVENT_CLICKED, (void *)noms[i]);
  }
}

void loop() {
  lv_timer_handler();   // Le "moteur" de LVGL : rendu, animations, entrées
  netTick();
  delay(5);
}