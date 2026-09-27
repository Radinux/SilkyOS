#include <Arduino.h>
#include "../App.h"
#include "../Radio.h"
#include "../Theme.h"
#include "../Widgets.h"

// Fréquences du rouleau, en centaines de kHz : 875 = 87,5 MHz
static const uint16_t F_MIN   = 875;
static const uint16_t F_MAX   = 1080;
static const uint16_t NB_FREQ = F_MAX - F_MIN + 1;

static lv_obj_t   *rouleau, *labelNom, *barreSignal, *labelSignal, *labelQualite, *labelVolume, *btnPower;
static lv_timer_t *timerRadio = nullptr;
static char        options[NB_FREQ * 6 + 1] = "";   // "87.5\n87.6\n...\n108.0"

static const char *optionsFrequences() {
  if (options[0] == '\0') {                          // Construit une seule fois
    char *p = options;
    for (uint16_t f = F_MIN; f <= F_MAX; f++) {
      p += sprintf(p, f < F_MAX ? "%u.%u\n" : "%u.%u", f / 10, f % 10);
    }
  }
  return options;
}

// ---------- Affichage ----------
static void majBoutons() {
  if (radioEstAllumee()) boutonSetTexte(btnPower, LV_SYMBOL_POWER, COUL_ROUGE);
  else                   boutonSetTexte(btnPower, LV_SYMBOL_POWER, COUL_VERT);
}

static void majVolume() {
  lv_label_set_text_fmt(labelVolume, "%u / 63", radioVolume());
}

static void majSignal() {
  if (!radioEstAllumee()) {
    lv_bar_set_value(barreSignal, 0, LV_ANIM_ON);
    lv_label_set_text(labelSignal, "-");
    lv_label_set_text(labelQualite, "-");
    lv_label_set_text(labelNom, "Radio eteinte");
    return;
  }

  uint8_t rssi, snr;
  radioSignal(&rssi, &snr);

  lv_bar_set_value(barreSignal, rssi > 60 ? 60 : rssi, LV_ANIM_ON);
  uint32_t c = rssi < 15 ? COUL_ROUGE : rssi < 30 ? COUL_ATTENTION : COUL_VERT;
  lv_obj_set_style_bg_color(barreSignal, lv_color_hex(c), LV_PART_INDICATOR);

  lv_label_set_text_fmt(labelSignal, "%u dBuV", rssi);
  lv_label_set_text_fmt(labelQualite, "SNR %u dB", snr);

  const char *nom = radioNomStation();
  lv_label_set_text(labelNom, nom[0] ? nom : "");
}

// Toutes les 100 ms : accord en direct, RDS, et le signal une fois sur cinq
static void boucle(lv_timer_t *) {
  static uint8_t compteur = 0;

  // La radio suit le rouleau PENDANT qu'on le tourne (en mode édition)
  uint16_t f = (F_MIN + lv_roller_get_selected(rouleau)) * 10;
  if (f != radioFrequence()) {
    radioSetFrequence(f);
    lv_label_set_text(labelNom, "");
  }

  radioTick();
  if (++compteur >= 5) { compteur = 0; majSignal(); }
}

// ---------- Boutons ----------
static void powerCb(lv_event_t *) {
  if (radioEstAllumee()) {
    radioEteindre();
  } else if (!radioAllumer()) {
    lv_label_set_text(labelNom, "SI4732 introuvable");
    return;
  }
  majBoutons();
  majSignal();
}

static void moinsCb(lv_event_t *) {
  uint8_t v = radioVolume();
  radioSetVolume(v > 4 ? v - 4 : 0);
  majVolume();
}

static void plusCb(lv_event_t *) {
  radioSetVolume(radioVolume() + 4);
  majVolume();
}

// ---------- API de l'app ----------
void radioCreate(lv_obj_t *contenu) {
  lv_obj_t *gauche, *droite;
  creerColonnes(contenu, &gauche, &droite);

  // --- Gauche : la station et le rouleau de fréquence ---
  lv_obj_t *heros = creerCarte(gauche);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(heros, 4, 0);

  labelNom = lv_label_create(heros);
  lv_obj_set_style_text_font(labelNom, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(labelNom, lv_color_hex(COUL_ACCENT), 0);

  rouleau = lv_roller_create(heros);
  lv_roller_set_options(rouleau, optionsFrequences(), LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(rouleau, 3);
  lv_obj_set_width(rouleau, lv_pct(80));
  lv_obj_set_style_text_font(rouleau, &lv_font_montserrat_28, 0);
  lv_obj_set_style_border_width(rouleau, 0, 0);
  lv_obj_set_style_bg_color(rouleau, lv_color_hex(COUL_CARTE_FOCUS), 0);
  lv_obj_set_style_text_color(rouleau, lv_color_hex(COUL_TEXTE_2), 0);
  lv_obj_set_style_bg_color(rouleau, lv_color_hex(COUL_ACCENT), LV_PART_SELECTED);
  lv_obj_set_style_text_color(rouleau, lv_color_hex(COUL_TEXTE), LV_PART_SELECTED);

  uint16_t f = radioFrequence() / 10;            // Dizaines de kHz → centaines de kHz
  if (f < F_MIN || f > F_MAX) f = 1000;
  lv_roller_set_selected(rouleau, f - F_MIN, LV_ANIM_OFF);

  lv_obj_t *mhz = lv_label_create(heros);
  lv_label_set_text(mhz, "MHz");
  lv_obj_set_style_text_color(mhz, lv_color_hex(COUL_TEXTE_2), 0);

  // --- Droite : signal, volume, boutons ---
  barreSignal = creerJauge(droite, "Signal", &labelSignal);
  lv_bar_set_range(barreSignal, 0, 60);
  labelQualite = creerInfo(droite, "Qualite");
  labelVolume  = creerInfo(droite, "Volume");

  lv_obj_t *rangee = creerRangeeVide(droite);
  creerBoutonAction(rangee, LV_SYMBOL_VOLUME_MID, COUL_TEXTE, moinsCb);
  btnPower = creerBoutonAction(rangee, LV_SYMBOL_POWER, COUL_VERT, powerCb);
  creerBoutonAction(rangee, LV_SYMBOL_VOLUME_MAX, COUL_TEXTE, plusCb);

  majBoutons();
  majVolume();
  majSignal();
  lv_group_focus_obj(btnPower);

  timerRadio = lv_timer_create(boucle, 100, nullptr);
}

void radioExit() {
  if (timerRadio) { lv_timer_delete(timerRadio); timerRadio = nullptr; }
  radioSauver();          // La radio continue de jouer : on mémorise juste les réglages
}