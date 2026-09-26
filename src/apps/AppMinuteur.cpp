#include <Arduino.h>
#include <string.h>
#include "../App.h"
#include "../Theme.h"
#include "../Ui.h"
#include "../Widgets.h"

enum { ARRETE, EN_MARCHE, EN_PAUSE };

// ---------- État : conservé même app fermée (le minuteur tourne en fond) ----------
static uint8_t  etat         = ARRETE;
static uint32_t fin          = 0;      // millis() de fin (EN_MARCHE)
static uint32_t restantPause = 0;      // ms restantes (EN_PAUSE)
static uint32_t duree        = 0;      // Durée réglée (ms), pour la barre
static uint8_t  regMin       = 5;      // Dernier réglage, retrouvé à la prochaine ouverture
static uint8_t  regSec       = 0;
static bool     ecranOuvert  = false;  // L'app est-elle affichée ? (sinon, ne pas toucher aux widgets)

static lv_obj_t   *blocReglage, *rollerMin, *rollerSec;
static lv_obj_t   *blocDecompte, *labelDecompte, *barreDecompte;
static lv_obj_t   *btnAnnuler, *btnPrincipal;
static lv_timer_t *timerMinuteur = nullptr;

static uint32_t restant() {
  if (etat == EN_MARCHE) {
    int32_t r = (int32_t)(fin - millis());
    return r > 0 ? r : 0;
  }
  return etat == EN_PAUSE ? restantPause : 0;
}

// "00\n01\n...\nNN" pour les rouleaux (construit une seule fois)
static const char *options(char *buf, int n) {
  if (buf[0] == '\0') {
    for (int i = 0; i < n; i++) {
      char item[4];
      snprintf(item, sizeof(item), i < n - 1 ? "%02d\n" : "%02d", i);
      strcat(buf, item);
    }
  }
  return buf;
}
static char optsMin[100 * 3] = "";
static char optsSec[60 * 3]  = "";

// ---------- Affichage ----------
static void majAffichage() {
  bool reglage = (etat == ARRETE);
  lv_obj_set_hidden(blocReglage,  !reglage);
  lv_obj_set_hidden(blocDecompte, reglage);
  lv_obj_set_hidden(btnAnnuler,   reglage);   // Un objet caché est aussi sauté par l'encodeur

  if (etat == EN_MARCHE) boutonSetTexte(btnPrincipal, LV_SYMBOL_PAUSE, COUL_ATTENTION);
  else                   boutonSetTexte(btnPrincipal, LV_SYMBOL_PLAY,  COUL_VERT);
}

static void majDecompte(lv_timer_t *) {
  if (etat == ARRETE) return;

  uint32_t r = restant();
  uint32_t s = (r + 999) / 1000;            // Arrondi au-dessus : "00:01" jusqu'au bout
  lv_label_set_text_fmt(labelDecompte, "%02lu:%02lu",
                        (unsigned long)(s / 60), (unsigned long)(s % 60));
  lv_bar_set_value(barreDecompte, duree ? (int)((uint64_t)r * 1000 / duree) : 0, LV_ANIM_OFF);
}

// Rouleau façon iOS : 3 lignes visibles, la sélection en couleur d'accent
static lv_obj_t *creerRouleau(lv_obj_t *parent, const char *opts, uint8_t sel) {
  lv_obj_t *r = lv_roller_create(parent);
  lv_roller_set_options(r, opts, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(r, 3);
  lv_roller_set_selected(r, sel, LV_ANIM_OFF);
  lv_obj_set_width(r, 48);                  // 2 × 48 + ":" tiennent dans la carte
  lv_obj_set_style_text_font(r, &lv_font_montserrat_20, 0);
  lv_obj_set_style_border_width(r, 0, 0);
  lv_obj_set_style_bg_color(r, lv_color_hex(COUL_CARTE_FOCUS), 0);
  lv_obj_set_style_text_color(r, lv_color_hex(COUL_TEXTE_2), 0);
  lv_obj_set_style_bg_color(r, lv_color_hex(COUL_ACCENT), LV_PART_SELECTED);
  lv_obj_set_style_text_color(r, lv_color_hex(COUL_TEXTE), LV_PART_SELECTED);
  return r;
}

// ---------- Boutons ----------
static void principalCb(lv_event_t *) {       // Démarrer / Pause / Reprendre
  switch (etat) {
    case ARRETE:
      regMin = lv_roller_get_selected(rollerMin);
      regSec = lv_roller_get_selected(rollerSec);
      duree  = (regMin * 60UL + regSec) * 1000UL;
      if (duree == 0) return;                 // Rien à décompter
      fin  = millis() + duree;
      etat = EN_MARCHE;
      break;
    case EN_MARCHE:
      restantPause = restant();
      etat = EN_PAUSE;
      break;
    case EN_PAUSE:
      fin  = millis() + restantPause;
      etat = EN_MARCHE;
      break;
  }
  majAffichage();
  majDecompte(nullptr);
}

static void annulerCb(lv_event_t *) {
  etat = ARRETE;
  majAffichage();
  lv_group_focus_obj(btnPrincipal);           // Le bouton Annuler vient de disparaître
}

// ---------- Travail de fond : appelé en permanence, app ouverte ou non ----------
void minuteurFond() {
  if (etat == EN_MARCHE && restant() == 0) {
    etat = ARRETE;
    if (ecranOuvert) majAffichage();
    uiAlerte("Termine !", "Minuteur");
  }
}

// ---------- API de l'app ----------
void minuteurCreate(lv_obj_t *contenu) {
  ecranOuvert = true;

  lv_obj_t *gauche, *droite;
  creerColonnes(contenu, &gauche, &droite);

  // --- Gauche : le réglage ou le décompte ---
  lv_obj_t *heros = creerCarte(gauche);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Mode réglage : deux rouleaux "mm : ss"
  blocReglage = creerRangeeVide(heros);
  lv_obj_set_style_pad_column(blocReglage, 4, 0);
  rollerMin = creerRouleau(blocReglage, options(optsMin, 100), regMin);
  lv_obj_t *sep = lv_label_create(blocReglage);
  lv_label_set_text(sep, ":");
  lv_obj_set_style_text_font(sep, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sep, lv_color_hex(COUL_TEXTE), 0);
  rollerSec = creerRouleau(blocReglage, options(optsSec, 60), regSec);

  // Mode décompte : le temps restant en grand + une barre qui se vide
  blocDecompte = lv_obj_create(heros);
  lv_obj_set_size(blocDecompte, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(blocDecompte, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(blocDecompte, 0, 0);
  lv_obj_set_style_pad_all(blocDecompte, 0, 0);
  lv_obj_set_style_pad_row(blocDecompte, 8, 0);
  lv_obj_set_scrollable(blocDecompte, false);
  lv_obj_set_flex_flow(blocDecompte, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(blocDecompte, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  labelDecompte = lv_label_create(blocDecompte);
  lv_obj_set_style_text_font(labelDecompte, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(labelDecompte, lv_color_hex(COUL_TEXTE), 0);

  barreDecompte = lv_bar_create(blocDecompte);
  lv_obj_set_size(barreDecompte, lv_pct(90), 6);
  lv_bar_set_range(barreDecompte, 0, 1000);
  lv_obj_set_style_bg_color(barreDecompte, lv_color_hex(COUL_CARTE_FOCUS), 0);
  lv_obj_set_style_bg_color(barreDecompte, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);

  // --- Droite : les boutons ---
  lv_obj_t *rangee = creerRangeeVide(droite);
  btnAnnuler   = creerBoutonAction(rangee, LV_SYMBOL_CLOSE, COUL_ROUGE, annulerCb);
  btnPrincipal = creerBoutonAction(rangee, LV_SYMBOL_PLAY,  COUL_VERT,  principalCb);

  majAffichage();
  majDecompte(nullptr);
  lv_group_focus_obj(etat == ARRETE ? rollerMin : btnPrincipal);

  timerMinuteur = lv_timer_create(majDecompte, 200, nullptr);
}

void minuteurExit() {
  ecranOuvert = false;
  if (timerMinuteur) { lv_timer_delete(timerMinuteur); timerMinuteur = nullptr; }
  // L'état reste : le minuteur continue, et minuteurFond() surveille la fin
}