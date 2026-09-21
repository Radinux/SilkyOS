#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Lvgl.h"

static const int LIGNES_BUFFER = 40;     // LVGL rend l'écran par bandes de 40 lignes
static lv_display_t *disp = nullptr;
static lv_group_t *groupe = nullptr;

// ---------- Horloge et affichage ----------
static uint32_t tickMillis() { return millis(); }

static void flushCb(lv_display_t *d, const lv_area_t *area, uint8_t *px) {
  displayFlush(area->x1, area->y1,
               lv_area_get_width(area), lv_area_get_height(area),
               (uint16_t *)px);
  lv_display_flush_ready(d);
}

// ---------- Entrées : encodeur + bouton ----------
static ButtonTracker bouton;

static bool     evtRetour  = false;
static bool     evtMenu    = false;
static bool     presse     = false;
static uint32_t debutAppui = 0;

static void encoderReadCb(lv_indev_t *indev, lv_indev_data_t *data) {
  static bool relacherAuProchain = false;
  static bool longTraite = false;

  int32_t diff = encoderGetDelta();

  // En navigation (pas en édition d'un slider) : un cran à la fois,
  // et rien du tout tant qu'une animation (scroll, transition) est en cours
  if (diff != 0 && !lv_group_get_editing(groupe)) {
    if (lv_anim_count_running() > 0) diff = 0;
    else if (diff > 1)               diff = 1;
    else if (diff < -1)              diff = -1;
  }
  data->enc_diff = diff;

  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  // Suivi de l'appui (barre de progression)
  if (btn.isPressed && !presse) debutAppui = millis();
  presse = btn.isPressed;

  // Moyen et long : pour notre framework, jamais transmis à LVGL
  if (btn.wasShortPressed) evtRetour = true;
  if (btn.isLongPressed && !longTraite) { longTraite = true; evtMenu = true; }
  if (!btn.isPressed) longTraite = false;

  // Clic court : clic synthétique pour LVGL
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

// ---------- Initialisation ----------
void lvglInit() {
  lv_init();
  lv_tick_set_cb(tickMillis);

  int32_t w = displayWidth();
  int32_t h = displayHeight();

  disp = lv_display_create(w, h);
  lv_display_set_flush_cb(disp, flushCb);

  // Buffer dimensionné sur le plus grand côté : valable en portrait ET en paysage
  int32_t cote  = (w > h) ? w : h;
  size_t taille = cote * LIGNES_BUFFER * sizeof(uint16_t);
  void *buf = heap_caps_malloc(taille, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_display_set_buffers(disp, buf, nullptr, taille, LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Thème sombre
  lv_theme_t *theme = lv_theme_default_init(disp,
      lv_palette_main(LV_PALETTE_CYAN),
      lv_palette_main(LV_PALETTE_ORANGE),
      true,
      LV_FONT_DEFAULT);
  lv_display_set_theme(disp, theme);

  // Encodeur comme périphérique d'entrée, rattaché au groupe par défaut
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(indev, encoderReadCb);

  groupe = lv_group_create();          // ← plus de "lv_group_t *" devant
  lv_group_set_default(groupe);
  lv_indev_set_group(indev, groupe);
}

// ---------- API ----------
void lvglSetRotation(uint8_t rotation) {
  displaySetRotation(rotation);                                      // L'écran tourne...
  lv_display_set_resolution(disp, displayWidth(), displayHeight());  // ...LVGL réorganise
}

bool     lvglPopRetour()    { bool e = evtRetour; evtRetour = false; return e; }
bool     lvglPopMenu()      { bool e = evtMenu;   evtMenu   = false; return e; }
bool     lvglBoutonPresse() { return presse; }
uint32_t lvglDebutAppui()   { return debutAppui; }