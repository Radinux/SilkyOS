#include <lvgl.h>
#include "../App.h"
#include "../Storage.h"
#include "../Theme.h"
#include "../Ui.h"
#include "UiInterne.h"

static void clicCb(lv_event_t *e) {
  uiOuvrirApp((int8_t)(intptr_t)lv_event_get_user_data(e), uiAnimEntree());
}

// Pastille colorée avec l'icône de l'app (commune à la liste et à la grille)
static lv_obj_t *creerPastille(lv_obj_t *parent, uint8_t i, int32_t taille,
                               int32_t rayon, const lv_font_t *police) {
  lv_obj_t *p = lv_obj_create(parent);
  lv_obj_set_size(p, taille, taille);
  lv_obj_set_style_radius(p, rayon, 0);
  lv_obj_set_style_bg_color(p, lv_color_hex(apps[i].couleur), 0);
  lv_obj_set_style_border_width(p, 0, 0);
  lv_obj_set_style_pad_all(p, 0, 0);
  lv_obj_set_scrollable(p, false);

  lv_obj_t *icone = lv_label_create(p);
  lv_label_set_text(icone, apps[i].icone);
  lv_obj_set_style_text_font(icone, police, 0);
  lv_obj_set_style_text_color(icone, lv_color_white(), 0);
  lv_obj_center(icone);
  return p;
}

// ---------- Style liste : "[pastille] Nom ........ >" ----------
static lv_obj_t *creerLigneApp(lv_obj_t *parent, uint8_t i) {
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
  themeCarte(btn);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(btn, 10, 0);

  creerPastille(btn, i, 30, 8, &lv_font_montserrat_14);

  lv_obj_t *nom = lv_label_create(btn);
  lv_label_set_text(nom, apps[i].nom);
  lv_obj_set_flex_grow(nom, 1);                  // Pousse le chevron à droite

  lv_obj_t *chevron = lv_label_create(btn);
  lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron, lv_color_hex(COUL_TEXTE_2), 0);
  return btn;
}

// ---------- Style grille : grosse pastille + nom dessous ----------
static lv_obj_t *creerTuileApp(lv_obj_t *parent, uint8_t i) {
  lv_obj_t *btn = lv_button_create(parent);
  // 2 tuiles par rangée en portrait, 4 en paysage
  lv_obj_set_size(btn, uiPaysage() ? lv_pct(23) : lv_pct(46), LV_SIZE_CONTENT);

  // Tuile transparente au repos, avec une bordure permanente mais invisible
  // (la largeur ne change jamais au focus : aucun texte ne saute de ligne)
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(btn, 14, 0);
  lv_obj_set_style_pad_ver(btn, 8, 0);
  lv_obj_set_style_pad_hor(btn, 2, 0);
  lv_obj_set_style_pad_row(btn, 6, 0);

  // Au focus : fond éclairé + bordure visible (sur les DEUX états de focus)
  const lv_style_selector_t etats[] = { LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY };
  for (lv_style_selector_t s : etats) {
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, s);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COUL_CARTE_FOCUS), s);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, s);
    lv_obj_set_style_border_color(btn, lv_color_hex(COUL_ACCENT), s);
    lv_obj_set_style_outline_width(btn, 0, s);
  }

  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  creerPastille(btn, i, 48, 14, &lv_font_montserrat_20);

  lv_obj_t *nom = lv_label_create(btn);
  lv_label_set_text(nom, apps[i].nom);
  lv_obj_set_style_text_font(nom, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(nom, lv_color_hex(COUL_TEXTE), 0);
  return btn;
}

// ---------- Affichage ----------
void launcherAfficher(int8_t selection, lv_screen_load_anim_t anim) {
  lv_obj_t *ecran;
  lv_obj_t *contenu = uiCreerEcran(nullptr, &ecran);     // nullptr = logo SilkyOS en titre

  bool grille = (reglages.launcher == 1);
  if (grille) {
    // Les tuiles se rangent toutes seules et passent à la ligne quand la rangée est pleine
    lv_obj_set_flex_flow(contenu, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(contenu, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  }

  for (uint8_t i = 0; i < NB_APPS; i++) {
    lv_obj_t *btn = grille ? creerTuileApp(contenu, i) : creerLigneApp(contenu, i);
    lv_obj_add_event_cb(btn, clicCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    if (i == selection) lv_group_focus_obj(btn);   // Retour sur l'app qu'on vient de quitter
  }

  uiChargerEcran(ecran, contenu, anim);
}