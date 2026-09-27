#include <lvgl.h>
#include "../App.h"
#include "../Icones.h"
#include "../Storage.h"
#include "../Theme.h"
#include "../Ui.h"
#include "UiInterne.h"

// Rebond au focus : la tuile se soulève de quelques pixels, avec un léger dépassement.
// Un décalage (translate) ne demande pas de calque, contrairement à un zoom : bien plus léger.
static const lv_style_prop_t PROPS_REBOND[] = {
  LV_STYLE_TRANSLATE_Y, LV_STYLE_PROP_INV                 // INV = fin de liste
};
static lv_style_transition_dsc_t transRebond;
static bool transPrete = false;

// Grille : colonnes de largeur égale (2 en portrait, 4 en paysage), rangées à la hauteur du contenu.
// LVGL garde un pointeur vers ces tableaux : ils doivent vivre aussi longtemps que l'écran (static).
static const int32_t COLS_PORTRAIT[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static const int32_t COLS_PAYSAGE[]  = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                         LV_GRID_TEMPLATE_LAST };
static const uint8_t MAX_RANGEES = 16;
static int32_t rangees[MAX_RANGEES + 1];

static void clicCb(lv_event_t *e) {
  uiOuvrirApp((int8_t)(intptr_t)lv_event_get_user_data(e), uiAnimEntree());
}

// Pastille pleine de la couleur de l'app, icône blanche.
// lueur = halo coloré dessous (en grille seulement : il faut de la place autour)
static lv_obj_t *creerPastille(lv_obj_t *parent, uint8_t i, int32_t taille, int32_t rayon,
                               const lv_font_t *police, bool lueur) {
  lv_color_t couleur = lv_color_hex(apps[i].couleur);

  lv_obj_t *p = lv_obj_create(parent);
  lv_obj_set_size(p, taille, taille);
  lv_obj_set_style_radius(p, rayon, 0);
  lv_obj_set_style_bg_color(p, couleur, 0);
  lv_obj_set_style_border_width(p, 0, 0);
  lv_obj_set_style_pad_all(p, 0, 0);
  lv_obj_set_scrollable(p, false);

  if (lueur) {
    lv_obj_set_style_shadow_width(p, 12, 0);
    lv_obj_set_style_shadow_color(p, couleur, 0);
    lv_obj_set_style_shadow_opa(p, LV_OPA_50, 0);
    lv_obj_set_style_shadow_offset_y(p, 3, 0);
  }

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

  creerPastille(btn, i, 30, 8, &silky_icones_20, false);

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
  lv_obj_set_height(btn, LV_SIZE_CONTENT);       // La largeur, c'est la colonne de la grille qui la donne

  // Tuile transparente au repos, bordure permanente mais invisible (largeur constante)
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(btn, 14, 0);
  lv_obj_set_style_pad_ver(btn, 8, 0);
  lv_obj_set_style_pad_hor(btn, 2, 0);
  lv_obj_set_style_pad_row(btn, 6, 0);
  lv_obj_set_style_transition(btn, &transRebond, 0);            // Retour à la normale

  // Au focus : fond éclairé, bordure visible, et la tuile se soulève de 4 px en rebondissant
  const lv_style_selector_t etats[] = { LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY };
  for (lv_style_selector_t s : etats) {
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, s);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COUL_CARTE_FOCUS), s);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, s);
    lv_obj_set_style_border_color(btn, lv_color_hex(COUL_ACCENT), s);
    lv_obj_set_style_outline_width(btn, 0, s);
    lv_obj_set_style_translate_y(btn, -4, s);
    lv_obj_set_style_transition(btn, &transRebond, s);          // Arrivée du focus
  }

  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  creerPastille(btn, i, 48, 14, &silky_icones_32, true);

  lv_obj_t *nom = lv_label_create(btn);
  lv_label_set_text(nom, apps[i].nom);
  lv_obj_set_style_text_font(nom, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(nom, lv_color_hex(COUL_TEXTE), 0);
  return btn;
}

// ---------- Affichage ----------
void launcherAfficher(int8_t selection, lv_screen_load_anim_t anim) {
  // La description de transition doit exister tant que les tuiles vivent : static, créée une fois
  if (!transPrete) {
    lv_style_transition_dsc_init(&transRebond, PROPS_REBOND, lv_anim_path_overshoot, 250, 0, nullptr);
    transPrete = true;
  }

  lv_obj_t *ecran;
  lv_obj_t *contenu = uiCreerEcran(nullptr, &ecran);     // nullptr = logo SilkyOS en titre

  bool grille = (reglages.launcher == 1);
  uint8_t nbCols = uiPaysage() ? 4 : 2;

  if (grille) {
    uint8_t nbRangees = (NB_APPS + nbCols - 1) / nbCols;       // Division arrondie au-dessus
    if (nbRangees > MAX_RANGEES) nbRangees = MAX_RANGEES;
    for (uint8_t r = 0; r < nbRangees; r++) rangees[r] = LV_GRID_CONTENT;
    rangees[nbRangees] = LV_GRID_TEMPLATE_LAST;

    lv_obj_set_grid_dsc_array(contenu, uiPaysage() ? COLS_PAYSAGE : COLS_PORTRAIT, rangees);
    lv_obj_set_style_pad_column(contenu, 6, 0);
    lv_obj_set_style_pad_row(contenu, 8, 0);
  }

  for (uint8_t i = 0; i < NB_APPS; i++) {
    lv_obj_t *btn = grille ? creerTuileApp(contenu, i) : creerLigneApp(contenu, i);
    lv_obj_add_event_cb(btn, clicCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

    // Chaque tuile dans sa case : les rangées incomplètes restent alignées à gauche
    if (grille) {
      lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % nbCols, 1,
                                LV_GRID_ALIGN_START,   i / nbCols, 1);
    }
    if (i == selection) lv_group_focus_obj(btn);   // Retour sur l'app qu'on vient de quitter
  }

  uiChargerEcran(ecran, contenu, anim);
}