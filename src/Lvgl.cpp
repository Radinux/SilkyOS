#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "Config.h"
#include "Display.h"
#include "Encoder.h"
#include "Button.h"
#include "Lvgl.h"
#include "Theme.h"

static const int LIGNES_BUFFER = 40;     // LVGL rend l'écran par bandes de 40 lignes
static lv_display_t *disp   = nullptr;
static lv_group_t   *groupe = nullptr;

// ---------- Horloge et affichage ----------
static uint32_t tickMillis() { return millis(); }

static void flushCb(lv_display_t *d, const lv_area_t *area, uint8_t *px) {
  displayFlush(area->x1, area->y1,
               lv_area_get_width(area), lv_area_get_height(area),
               (uint16_t *)px);
  lv_display_flush_ready(d);
}

// ---------- État des entrées ----------
static ButtonTracker bouton;

static bool     evtRetour        = false;
static bool     evtMenu          = false;
static int32_t  evtScroll        = 0;
static bool     presse           = false;
static uint32_t debutAppui       = 0;

static bool     enVeille         = false;
static bool     evtReveil        = false;
static uint32_t derniereActivite = 0;

static bool     capture          = false;
static int32_t  evtRotation      = 0;
static bool     evtClic          = false;

// ---------- Outils de navigation ----------
// Un objet est-il caché, lui ou l'un de ses parents ?
static bool estCache(lv_obj_t *o) {
  for (; o; o = lv_obj_get_parent(o)) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return true;
  }
  return false;
}

// Le focus est-il sur le premier (ou le dernier) widget VISIBLE du groupe ?
static bool focusAuBout(bool fin) {
  lv_obj_t *focus = lv_group_get_focused(groupe);
  uint32_t n = lv_group_get_obj_count(groupe);

  for (uint32_t k = 0; k < n; k++) {
    lv_obj_t *o = lv_group_get_obj_by_index(groupe, fin ? n - 1 - k : k);
    if (!estCache(o)) return o == focus;    // Premier widget visible depuis ce bout
  }
  return true;
}

// La page qui contient le focus peut-elle encore défiler dans ce sens ?
static bool peutDefiler(bool versBas) {
  lv_obj_t *focus = lv_group_get_focused(groupe);
  if (!focus) return false;

  // On remonte jusqu'au premier parent défilable : la zone de contenu de l'écran
  for (lv_obj_t *o = lv_obj_get_parent(focus); o; o = lv_obj_get_parent(o)) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_SCROLLABLE)) {
      return versBas ? lv_obj_get_scroll_bottom(o) > 0 : lv_obj_get_scroll_top(o) > 0;
    }
  }
  return false;
}

// ---------- Lecture de l'encodeur (appelée par LVGL) ----------
static void encoderReadCb(lv_indev_t *indev, lv_indev_data_t *data) {
  static bool relacherAuProchain = false;
  static bool longTraite = false;
  static bool avaler = false;      // Jette le geste qui a servi à réveiller l'écran

  int32_t diff = encoderGetDelta();
  bool enfonce = (digitalRead(ENCODER_PUSH_BUTTON) == LOW);
  ButtonTracker::State btn = bouton.update(enfonce);

  // ---- Activité : alimente le compteur de veille ----
  bool activite = (diff != 0) || btn.isPressed;
  if (activite) derniereActivite = millis();

  if (enVeille && activite) {
    evtReveil = true;
    avaler = true;
  }

  // ---- Geste de réveil : il rallume l'écran, et rien d'autre ----
  if (avaler) {
    data->enc_diff = 0;
    data->state = LV_INDEV_STATE_RELEASED;
    relacherAuProchain = false;
    presse = false;
    if (!btn.isPressed) avaler = false;   // Libéré seulement au relâchement du bouton
    return;
  }

  // ---- Rotation ----
  if (capture) {
    evtRotation += diff;                      // Un jeu prend les crans pour lui
    diff = 0;
  } else if (lv_group_get_obj_count(groupe) == 0) {
    diff = (diff > 0) ? 1 : -1;               // Un cran à la fois
    bool versBas = (diff > 0);

    if (focusAuBout(versBas) && peutDefiler(versBas)) {
      // Au bout des widgets, mais il reste de la page à voir : on la fait défiler
      evtScroll += diff;
      diff = 0;
    } else if (lv_anim_count_running() > 0) {
      diff = 0;                               // Rien pendant une animation
    }
    // Sinon, LVGL déplace le focus, et reboucle au début ou à la fin si besoin
  }

  data->enc_diff = diff;

  // ---- Bouton ----
  if (btn.isPressed && !presse) debutAppui = millis();
  presse = btn.isPressed;

  // Moyen et long : pour notre framework, jamais transmis à LVGL
  if (btn.wasShortPressed) evtRetour = true;
  if (btn.isLongPressed && !longTraite) { longTraite = true; evtMenu = true; }
  if (!btn.isPressed) longTraite = false;

  // Clic court : pour le jeu qui a capturé l'encodeur, sinon clic synthétique pour LVGL
  if (capture) {
    if (btn.wasClicked) evtClic = true;
    data->state = LV_INDEV_STATE_RELEASED;
    relacherAuProchain = false;
  } else if (relacherAuProchain) {
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

  themeInit();          // Palette sauvegardée + styles maison

  // Encodeur comme périphérique d'entrée, rattaché au groupe par défaut
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(indev, encoderReadCb);

  groupe = lv_group_create();
  lv_group_set_default(groupe);
  lv_indev_set_group(indev, groupe);

  derniereActivite = millis();    // Le compte à rebours de veille démarre au boot
}

// ---------- API ----------
void lvglSetRotation(uint8_t rotation) {
  displaySetRotation(rotation);                                      // L'écran tourne...
  lv_display_set_resolution(disp, displayWidth(), displayHeight());  // ...LVGL réorganise
}

bool     lvglPopRetour()    { bool e = evtRetour; evtRetour = false; return e; }
bool     lvglPopMenu()      { bool e = evtMenu;   evtMenu   = false; return e; }
int32_t  lvglPopScroll()    { int32_t d = evtScroll; evtScroll = 0; return d; }
bool     lvglBoutonPresse() { return presse; }
uint32_t lvglDebutAppui()   { return debutAppui; }

// ---------- Veille ----------
uint32_t lvglInactivite() { return millis() - derniereActivite; }

void lvglSetVeille(bool v) {
  enVeille = v;
  if (!v) derniereActivite = millis();   // Au réveil, le compte à rebours repart de zéro
}

bool lvglPopReveil() { bool e = evtReveil; evtReveil = false; return e; }

// ---------- Capture (jeux) ----------
void lvglCaptureEncodeur(bool active) {
  capture = active;
  evtRotation = 0;
  evtClic = false;
}

int32_t lvglPopRotation() { int32_t r = evtRotation; evtRotation = 0; return r; }
bool    lvglPopClic()     { bool c = evtClic; evtClic = false; return c; }