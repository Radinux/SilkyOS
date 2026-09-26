#include <Arduino.h>
#include <lvgl.h>
#include "../Battery.h"
#include "../Clock.h"
#include "../Display.h"
#include "../Lvgl.h"
#include "../Storage.h"
#include "../Theme.h"
#include "../Ui.h"
#include "UiInterne.h"

// Durées en secondes, dans le même ordre que uiVeilleOptions() (0 = jamais)
static const uint16_t DUREES[]   = { 0, 15, 30, 60, 120, 300 };
static const uint8_t  NB_DUREES  = sizeof(DUREES) / sizeof(DUREES[0]);

static const uint8_t  LUMI_AOD   = 38;      // ~15 % de 255
static const uint32_t CPU_VEILLE = 80;      // MHz : le minimum compatible avec le WiFi
static const uint32_t CPU_NORMAL = 240;

static bool      enVeille = false;
static lv_obj_t *voileAod = nullptr, *heureAod, *dateAod, *battAod;
static int       derniereMinuteAod = -2;

const char *uiVeilleOptions() {
  return "Jamais\n15 s\n30 s\n1 min\n2 min\n5 min";
}

bool uiEnVeille() { return enVeille; }

// ---------- AOD : une horloge minimaliste sur le calque supérieur ----------
static void creerAod() {
  voileAod = uiCreerVoile();

  heureAod = lv_label_create(voileAod);
  lv_obj_set_style_text_font(heureAod, &lv_font_montserrat_48, 0);

  dateAod = lv_label_create(voileAod);
  battAod = lv_label_create(voileAod);
  lv_obj_set_style_text_font(battAod, &lv_font_montserrat_12, 0);

  lv_obj_set_hidden(voileAod, true);
}

// Ne redessine que quand la minute change : ~1 rafraîchissement par minute
static void majAod() {
  static const char *jours[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };
  struct tm t;
  int minute = clockGet(&t) ? t.tm_hour * 60 + t.tm_min : -1;
  if (minute == derniereMinuteAod) return;
  derniereMinuteAod = minute;

  if (minute < 0) {
    lv_label_set_text(heureAod, "--:--");
    lv_label_set_text(dateAod, "");
  } else {
    lv_label_set_text_fmt(heureAod, "%02d:%02d", t.tm_hour, t.tm_min);
    lv_label_set_text_fmt(dateAod, "%s %d", jours[t.tm_wday], t.tm_mday);
  }

  if (batteryOnUsb()) lv_label_set_text(battAod, LV_SYMBOL_CHARGE);
  else                lv_label_set_text_fmt(battAod, "%d%%", batteryPercent());
}

static void entrerVeille() {
  enVeille = true;
  lvglSetVeille(true);

  if (reglages.aod) {
    if (!voileAod) creerAod();

    // Couleurs du thème actif, posées à chaque entrée (le thème a pu changer)
    lv_obj_set_style_bg_color(voileAod, lv_color_hex(COUL_FOND), 0);
    lv_obj_set_style_text_color(heureAod, lv_color_hex(COUL_TEXTE), 0);
    lv_obj_set_style_text_color(dateAod,  lv_color_hex(COUL_ACCENT), 0);
    lv_obj_set_style_text_color(battAod,  lv_color_hex(COUL_TEXTE_2), 0);

    derniereMinuteAod = -2;                 // Force l'affichage immédiat
    majAod();
    lv_obj_set_hidden(voileAod, false);
    displaySetBrightness(LUMI_AOD);
  } else {
    displayEteindre();
  }

  setCpuFrequencyMhz(CPU_VEILLE);           // Moins vite = beaucoup moins de courant
}

void veilleReveiller() {
  if (!enVeille) return;
  enVeille = false;
  lvglSetVeille(false);

  setCpuFrequencyMhz(CPU_NORMAL);
  if (voileAod) lv_obj_set_hidden(voileAod, true);
  displaySetBrightness(reglages.luminosite);
}

// autorisee = false pendant une mise à jour ou une alerte : on reste (ou on se remet) allumé
void veilleMaj(bool autorisee) {
  if (enVeille) {
    if (lvglPopReveil() || !autorisee) veilleReveiller();
    else if (reglages.aod)             majAod();
    return;
  }

  uint16_t duree = DUREES[reglages.veille < NB_DUREES ? reglages.veille : 0];
  if (!autorisee || duree == 0) return;

  if (lvglInactivite() > duree * 1000UL) entrerVeille();
}