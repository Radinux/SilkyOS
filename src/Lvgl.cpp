#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "Display.h"
#include "Lvgl.h"
#include "Config.h"
#include "Encoder.h"
#include "Button.h"

static const int LIGNES_BUFFER = 40;   // LVGL rend l'écran par bandes de 40 lignes

static uint32_t tickMillis() { return millis(); }

// Appelé par LVGL quand une zone est prête à être affichée
static void flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px) {
  displayFlush(area->x1, area->y1,
               lv_area_get_width(area), lv_area_get_height(area),
               (uint16_t *)px);
  lv_display_flush_ready(disp);        // "C'est affiché, tu peux continuer"
}

static ButtonTracker bouton;


static bool     evtRetour  = false;
static bool     evtMenu    = false;
static bool     presse     = false;
static uint32_t debutAppui = 0;

static void encoderReadCb(lv_indev_t *indev, lv_indev_data_t *data) {
  static bool relacherAuProchain = false;
  static bool longTraite = false;

  data->enc_diff = encoderGetDelta();

  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  // Suivi de l'appui (pour la barre de progression)
  if (btn.isPressed && !presse) debutAppui = millis();
  presse = btn.isPressed;

  // Moyen et long : pour notre framework, jamais transmis à LVGL
  if (btn.wasShortPressed) evtRetour = true;
  if (btn.isLongPressed && !longTraite) { longTraite = true; evtMenu = true; }
  if (!btn.isPressed) longTraite = false;

  // Clic court : transmis à LVGL sous forme de clic synthétique
  if (relacherAuProchain) {
    data->state = LV_INDEV_STATE_RELEASED;
    relacherAuProchain = false;
  } else if (btn.wasClicked) {
    data->state = LV_INDEV_STATE_PRESSED;
    relacherAuProchain = true;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

bool     lvglPopRetour()    { bool e = evtRetour; evtRetour = false; return e; }
bool     lvglPopMenu()      { bool e = evtMenu;   evtMenu   = false; return e; }
bool     lvglBoutonPresse() { return presse; }
uint32_t lvglDebutAppui()   { return debutAppui; }

void lvglInit() {
  lv_init();
  lv_tick_set_cb(tickMillis);          // LVGL a besoin d'une horloge en ms

  int32_t w = displayWidth();
  int32_t h = displayHeight();

  lv_display_t *disp = lv_display_create(w, h);
  lv_display_set_flush_cb(disp, flushCb);

  // Buffer de rendu en RAM interne (rapide), pas besoin de plein écran
  size_t taille = w * LIGNES_BUFFER * sizeof(uint16_t);
  void *buf = heap_caps_malloc(taille, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_display_set_buffers(disp, buf, nullptr, taille, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_theme_t *theme = lv_theme_default_init(disp,
      lv_palette_main(LV_PALETTE_CYAN),      // Couleur principale (focus...)
      lv_palette_main(LV_PALETTE_ORANGE),    // Couleur secondaire
      true,                                  // Mode sombre
      LV_FONT_DEFAULT);
  lv_display_set_theme(disp, theme);

  // Encodeur déclaré comme périphérique d'entrée
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(indev, encoderReadCb);

  // Groupe par défaut : tout widget créé ensuite y est ajouté automatiquement
  lv_group_t *groupe = lv_group_create();
  lv_group_set_default(groupe);
  lv_indev_set_group(indev, groupe);
}

