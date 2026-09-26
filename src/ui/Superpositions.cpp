#include <Arduino.h>
#include <lvgl.h>
#include "../Network.h"
#include "../Theme.h"
#include "../Ui.h"
#include "UiInterne.h"

static lv_obj_t *creerTexte(lv_obj_t *parent, const lv_font_t *police, uint32_t couleur) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, police, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(couleur), 0);
  return l;
}

// ================= Écran de mise à jour =================
static lv_obj_t *voileMaj, *arcMaj, *pctArc, *barreMaj, *pctBarre, *etatMaj;

static void creerEcranMaj() {
  voileMaj = uiCreerVoile();

  lv_obj_t *titre = creerTexte(voileMaj, &lv_font_montserrat_20, 0xFFFFFF);
  lv_label_set_text(titre, "Mise a jour");

  // Portrait : anneau façon montre, pourcentage au centre
  arcMaj = lv_arc_create(voileMaj);
  lv_obj_set_size(arcMaj, 110, 110);
  lv_arc_set_rotation(arcMaj, 270);
  lv_arc_set_bg_angles(arcMaj, 0, 360);
  lv_arc_set_range(arcMaj, 0, 100);
  lv_obj_remove_style(arcMaj, nullptr, LV_PART_KNOB);
  lv_obj_set_style_arc_width(arcMaj, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arcMaj, 10, LV_PART_INDICATOR);
  lv_group_remove_obj(arcMaj);

  pctArc = creerTexte(arcMaj, &lv_font_montserrat_20, 0xFFFFFF);
  lv_obj_center(pctArc);

  // Paysage : fine barre, pourcentage en dessous
  barreMaj = lv_bar_create(voileMaj);
  lv_obj_set_size(barreMaj, lv_pct(60), 6);
  lv_bar_set_range(barreMaj, 0, 100);
  pctBarre = creerTexte(voileMaj, &lv_font_montserrat_20, 0xFFFFFF);

  etatMaj = lv_label_create(voileMaj);
  lv_obj_set_hidden(voileMaj, true);
}

// Lit l'état écrit par la tâche réseau : SEULE l'UI touche LVGL
static void majEcranMaj() {
  static bool     visible     = false;
  static uint8_t  dernierEtat = OTA_AUCUN;
  static uint8_t  dernierPct  = 255;
  static uint32_t debutEchec  = 0;

  uint8_t etat = netOtaEtat();

  if (etat == OTA_AUCUN) {
    if (visible) { lv_obj_set_hidden(voileMaj, true); visible = false; }
    dernierEtat = OTA_AUCUN;
    return;
  }

  if (!visible) {
    // Variante et couleurs choisies au moment où l'écran apparaît
    bool pay = uiPaysage();
    lv_obj_set_hidden(arcMaj,   pay);
    lv_obj_set_hidden(barreMaj, !pay);
    lv_obj_set_hidden(pctBarre, !pay);
    lv_obj_set_style_pad_row(voileMaj, pay ? 8 : 14, 0);
    lv_obj_set_style_bg_color(voileMaj, lv_color_hex(COUL_FOND), 0);
    lv_obj_set_style_arc_color(arcMaj, lv_color_hex(COUL_CARTE_FOCUS), LV_PART_MAIN);
    lv_obj_set_style_bg_color(barreMaj, lv_color_hex(COUL_CARTE_FOCUS), 0);

    lv_obj_set_hidden(voileMaj, false);
    visible = true;
    dernierPct = 255;
  }

  uint8_t p = netOtaPourcent();
  if (p != dernierPct) {
    dernierPct = p;
    lv_arc_set_value(arcMaj, p);
    lv_bar_set_value(barreMaj, p, LV_ANIM_ON);
    lv_label_set_text_fmt(pctArc,   "%d%%", p);
    lv_label_set_text_fmt(pctBarre, "%d%%", p);
  }

  if (etat != dernierEtat) {
    dernierEtat = etat;
    uint32_t c, cTexte;
    switch (etat) {
      case OTA_EN_COURS:
        lv_label_set_text(etatMaj, "Ne pas eteindre");
        c = COUL_ACCENT;  cTexte = COUL_TEXTE_2;
        break;
      case OTA_REUSSI:
        lv_label_set_text(etatMaj, "Redemarrage...");
        c = cTexte = COUL_VERT;
        break;
      default:
        lv_label_set_text(etatMaj, "Echec");
        c = cTexte = COUL_ROUGE;
        debutEchec = millis();
        break;
    }
    lv_obj_set_style_text_color(etatMaj, lv_color_hex(cTexte), 0);
    lv_obj_set_style_arc_color(arcMaj, lv_color_hex(c), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(barreMaj, lv_color_hex(c), LV_PART_INDICATOR);
  }

  // Après un échec, on laisse le message 3 s puis on rend la main
  if (etat == OTA_ECHEC && millis() - debutEchec > 3000) netOtaAcquitter();
}

// ================= Alertes =================
static lv_obj_t   *voileAlerte   = nullptr;
static lv_obj_t   *titreAlerte   = nullptr;
static lv_obj_t   *focusAvant    = nullptr;
static lv_timer_t *timerClignote = nullptr;

static void clignoteCb(lv_timer_t *) {
  static bool allume = true;
  allume = !allume;
  lv_obj_set_style_opa(titreAlerte, allume ? LV_OPA_COVER : LV_OPA_30, 0);
}

static void fermerAlerteCb(lv_event_t *) {
  lv_group_focus_freeze(lv_group_get_default(), false);   // On rend la navigation

  lv_timer_delete(timerClignote);
  timerClignote = nullptr;

  // "async" : on ne supprime pas un objet pendant qu'il est en train de nous appeler
  lv_obj_delete_async(voileAlerte);
  voileAlerte = nullptr;

  if (focusAvant) lv_group_focus_obj(focusAvant);
}

void uiAlerte(const char *titre, const char *texte) {
  if (voileAlerte) return;                        // Une seule alerte à la fois
  veilleReveiller();

  lv_group_t *g = lv_group_get_default();
  focusAvant = lv_group_get_focused(g);

  voileAlerte = uiCreerVoile();

  titreAlerte = creerTexte(voileAlerte, &lv_font_montserrat_28, COUL_ACCENT);
  lv_label_set_text(titreAlerte, titre);

  lv_obj_t *sousTitre = creerTexte(voileAlerte, &lv_font_montserrat_14, COUL_TEXTE_2);
  lv_label_set_text(sousTitre, texte);

  lv_obj_t *btn = lv_button_create(voileAlerte);   // Ajouté tout seul au groupe
  lv_obj_set_size(btn, lv_pct(60), LV_SIZE_CONTENT);
  themeCarte(btn);
  lv_obj_t *ok = lv_label_create(btn);
  lv_label_set_text(ok, "OK");
  lv_obj_center(ok);
  lv_obj_add_event_cb(btn, fermerAlerteCb, LV_EVENT_CLICKED, nullptr);

  // Focus verrouillé sur OK : tourner l'encodeur ne peut plus rien sélectionner d'autre
  lv_group_focus_obj(btn);
  lv_group_focus_freeze(g, true);

  timerClignote = lv_timer_create(clignoteCb, 500, nullptr);
}

// ================= API interne =================
void superpositionsCreer() { creerEcranMaj(); }
void superpositionsMaj()   { majEcranMaj(); }

bool superpositionBloque() {
  uint8_t ota = netOtaEtat();
  return voileAlerte || ota == OTA_EN_COURS || ota == OTA_REUSSI;
}