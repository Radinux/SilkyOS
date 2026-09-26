#include <Arduino.h>
#include <string.h>
#include "../App.h"
#include "../Theme.h"
#include "../Widgets.h"

static const uint8_t NB_TOURS_MAX = 5;

// ---------- État : conservé même app fermée (le chrono tourne en fond) ----------
static bool     enMarche         = false;
static uint32_t debut            = 0;     // millis() au dernier démarrage
static uint32_t accumule         = 0;     // Temps cumulé avant la dernière pause (ms)
static uint32_t tours[NB_TOURS_MAX];      // Durée des derniers tours, le plus récent en [0]
static uint8_t  nbTours          = 0;
static uint16_t numTour          = 0;     // Numéro du dernier tour enregistré
static uint32_t totalDernierTour = 0;     // Temps total au moment du dernier tour

// ---------- Affichage (recréé à chaque ouverture) ----------
static lv_obj_t   *labelTemps, *labelCs, *btnGauche, *btnDroite, *labelTours;
static lv_timer_t *timerChrono = nullptr;
static uint32_t    derniereSec = UINT32_MAX;

static uint32_t tempsTotal() {
  return accumule + (enMarche ? millis() - debut : 0);
}

static void majTemps(lv_timer_t *) {
  uint32_t t = tempsTotal();

  // Le gros label (48 px) ne change que toutes les secondes : on ne le redessine qu'alors
  uint32_t sec = t / 1000;
  if (sec != derniereSec) {
    derniereSec = sec;
    uint32_t min = sec / 60;
    if (min > 99) min = 99;
    lv_label_set_text_fmt(labelTemps, "%02lu:%02lu", (unsigned long)min, (unsigned long)(sec % 60));
  }
  lv_label_set_text_fmt(labelCs, ".%02lu", (unsigned long)((t / 10) % 100));
}

static void majBoutons() {
  if (enMarche) {
    boutonSetTexte(btnGauche, "Tour", COUL_TEXTE);
    boutonSetTexte(btnDroite, LV_SYMBOL_PAUSE, COUL_ATTENTION);
  } else {
    boutonSetTexte(btnGauche, LV_SYMBOL_REFRESH, COUL_TEXTE);
    boutonSetTexte(btnDroite, LV_SYMBOL_PLAY, COUL_VERT);
  }
}

static void majTours() {
  if (nbTours == 0) { lv_label_set_text(labelTours, "-"); return; }

  char texte[NB_TOURS_MAX * 28] = "";
  for (uint8_t i = 0; i < nbTours; i++) {
    uint32_t t = tours[i];
    char ligne[28];
    snprintf(ligne, sizeof(ligne), "%sTour %u   %02lu:%02lu.%02lu",
             i ? "\n" : "", numTour - i,
             (unsigned long)(t / 60000), (unsigned long)((t / 1000) % 60),
             (unsigned long)((t / 10) % 100));
    strcat(texte, ligne);
  }
  lv_label_set_text(labelTours, texte);
}

// ---------- Boutons ----------
static void droiteCb(lv_event_t *) {           // Démarrer / Pause
  if (enMarche) {
    accumule += millis() - debut;
    enMarche = false;
  } else {
    debut = millis();
    enMarche = true;
  }
  majBoutons();
}

static void gaucheCb(lv_event_t *) {           // Tour (en marche) / Réinitialiser (à l'arrêt)
  if (enMarche) {
    uint32_t t = tempsTotal();
    memmove(&tours[1], &tours[0], (NB_TOURS_MAX - 1) * sizeof(tours[0]));
    tours[0] = t - totalDernierTour;
    totalDernierTour = t;
    numTour++;
    if (nbTours < NB_TOURS_MAX) nbTours++;
  } else {
    accumule = 0;
    nbTours = 0;
    numTour = 0;
    totalDernierTour = 0;
  }
  majBoutons();
  majTours();
  majTemps(nullptr);
}

// ---------- API de l'app ----------
void chronoCreate(lv_obj_t *contenu) {
  derniereSec = UINT32_MAX;                    // Force l'affichage du temps dès l'ouverture

  lv_obj_t *gauche, *droite;
  creerColonnes(contenu, &gauche, &droite);

  // --- Gauche : le temps ---
  lv_obj_t *heros = creerCarte(gauche);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(heros, 0, 0);

  labelTemps = lv_label_create(heros);
  lv_obj_set_style_text_font(labelTemps, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(labelTemps, lv_color_hex(COUL_TEXTE), 0);

  labelCs = lv_label_create(heros);
  lv_obj_set_style_text_font(labelCs, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(labelCs, lv_color_hex(COUL_ACCENT), 0);

  // --- Droite : les commandes et les tours ---
  lv_obj_t *rangee = creerRangeeVide(droite);
  btnGauche = creerBoutonAction(rangee, "", COUL_TEXTE, gaucheCb);
  btnDroite = creerBoutonAction(rangee, "", COUL_VERT,  droiteCb);

  labelTours = creerInfo(droite, "Tours");
  lv_obj_set_style_text_font(labelTours, &lv_font_montserrat_12, 0);

  majBoutons();
  majTours();
  majTemps(nullptr);
  lv_group_focus_obj(btnDroite);               // Prêt à démarrer

  timerChrono = lv_timer_create(majTemps, 50, nullptr);   // 20 images/s pour les centièmes
}

void chronoExit() {
  if (timerChrono) { lv_timer_delete(timerChrono); timerChrono = nullptr; }
  // L'état (enMarche, accumule...) reste : le chrono continue de tourner
}