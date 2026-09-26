#include <Arduino.h>
#include "../App.h"
#include "../Clock.h"
#include "../Storage.h"
#include "../Theme.h"
#include "../Widgets.h"

static lv_obj_t   *labelHeure, *labelSec, *labelDate, *labelSync, *ddFuseau;
static lv_timer_t *timerHorloge = nullptr;
static int         derniereSec  = -1;

static const char *jours[] = { "Dimanche", "Lundi", "Mardi", "Mercredi",
                               "Jeudi", "Vendredi", "Samedi" };
static const char *mois[]  = { "janv.", "fevr.", "mars", "avril", "mai", "juin",
                               "juil.", "aout", "sept.", "oct.", "nov.", "dec." };

static void majHorloge(lv_timer_t *) {
  struct tm t;

  if (!clockGet(&t)) {
    lv_label_set_text(labelHeure, "--:--");
    lv_label_set_text(labelSec, "");
    lv_label_set_text(labelDate, "");
    lv_label_set_text(labelSync, "En attente du WiFi");
    derniereSec = -1;
    return;
  }

  if (t.tm_sec == derniereSec) return;      // Rien de neuf depuis la dernière fois
  derniereSec = t.tm_sec;

  lv_label_set_text_fmt(labelHeure, "%02d:%02d", t.tm_hour, t.tm_min);
  lv_label_set_text_fmt(labelSec, "%02d", t.tm_sec);
  lv_label_set_text_fmt(labelDate, "%s %d %s", jours[t.tm_wday], t.tm_mday, mois[t.tm_mon]);
  lv_label_set_text(labelSync, "Synchro NTP");
}

static void fuseauCb(lv_event_t *e) {
  reglages.fuseau = lv_dropdown_get_selected(ddFuseau);
  storageSave();
  derniereSec = -1;                         // Force le redessin immédiat
  majHorloge(nullptr);
}

void horlogeCreate(lv_obj_t *contenu) {
  lv_obj_t *gauche, *droite;
  creerColonnes(contenu, &gauche, &droite);

  // --- Gauche : l'heure en grand ---
  lv_obj_t *heros = creerCarte(gauche);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(heros, 2, 0);
  rendreConsultable(heros);       // Le fuseau est le seul widget : l'heure sert d'étape de retour

  labelHeure = lv_label_create(heros);
  lv_obj_set_style_text_font(labelHeure, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(labelHeure, lv_color_hex(COUL_TEXTE), 0);

  labelSec = lv_label_create(heros);
  lv_obj_set_style_text_font(labelSec, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(labelSec, lv_color_hex(COUL_ACCENT), 0);

  labelDate = lv_label_create(heros);
  lv_obj_set_style_text_color(labelDate, lv_color_hex(COUL_TEXTE_2), 0);

  labelSync = lv_label_create(heros);
  lv_obj_set_style_text_font(labelSync, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(labelSync, lv_color_hex(COUL_TEXTE_2), 0);

  // --- Droite : le choix du fuseau ---
  ddFuseau = creerDropdown(droite, "Fuseau", clockFuseauOptions(), fuseauCb);
  dropdownSetValeur(ddFuseau, reglages.fuseau);

  derniereSec = -1;
  majHorloge(nullptr);
  timerHorloge = lv_timer_create(majHorloge, 200, nullptr);
}

void horlogeExit() {
  if (timerHorloge) { lv_timer_delete(timerHorloge); timerHorloge = nullptr; }
}