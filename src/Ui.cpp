#include <Arduino.h>
#include <lvgl.h>
#include "App.h"
#include "Button.h"
#include "Lvgl.h"
#include "Theme.h"
#include "Ui.h"

static int8_t appActive = -1;        // -1 = menu principal
static int8_t selection = 0;         // Dernière app ouverte (focus au retour)

// Sous-page ouverte par l'app active (un seul niveau)
static bool  dansPage     = false;
static void (*pageExit)() = nullptr;

static lv_obj_t *barreAppui = nullptr;

// Rechargement de l'écran courant, demandé depuis un callback (changement de thème...)
static bool rechargementDemande = false;

void uiRecharger() {
  rechargementDemande = true;      // Traité dans uiUpdate(), hors de tout callback
}

// Défilement à l'encodeur pour les pages sans widget focusable
static lv_obj_t     *contenuDefilable = nullptr;
static int32_t       cibleScroll      = 0;
static const int32_t PAS_SCROLL       = 40;     // Pixels par cran

static void ouvrirApp(int8_t index, lv_screen_load_anim_t anim);

// ---------- Défilement des pages "lecture seule" ----------
// À appeler APRÈS la création des widgets d'un écran
static void activerDefilement(lv_obj_t *contenu) {
  cibleScroll = 0;
  bool vide = (lv_group_get_obj_count(lv_group_get_default()) == 0);
  contenuDefilable = vide ? contenu : nullptr;
}

static void majDefilement() {
  int32_t d = lvglPopScroll();
  if (d == 0 || contenuDefilable == nullptr) return;

  // Défilement max = position actuelle + ce qui reste en dessous
  int32_t max = lv_obj_get_scroll_y(contenuDefilable)
              + lv_obj_get_scroll_bottom(contenuDefilable);

  cibleScroll += d * PAS_SCROLL;
  if (cibleScroll < 0)   cibleScroll = 0;
  if (cibleScroll > max) cibleScroll = max;

  lv_obj_scroll_to_y(contenuDefilable, cibleScroll, LV_ANIM_ON);
}

// ---------- Écran type : titre + zone de contenu ----------
static lv_obj_t *creerEcran(const char *titre, lv_obj_t **ecranOut) {
  lv_obj_t *ecran = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(ecran, lv_color_hex(COUL_FOND), 0);
  lv_obj_set_flex_flow(ecran, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(ecran, 0, 0);
  lv_obj_set_style_pad_gap(ecran, 0, 0);

  // --- En-tête : juste un titre, sans barre (style montre) ---
  lv_obj_t *entete = lv_obj_create(ecran);
  lv_obj_set_size(entete, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(entete, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(entete, 0, 0);
  lv_obj_set_style_pad_top(entete, 10, 0);
  lv_obj_set_style_pad_bottom(entete, 2, 0);
  lv_obj_set_scrollable(entete, false);

  lv_obj_t *label = lv_label_create(entete);
  lv_label_set_text(label, titre);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COUL_TEXTE), 0);
  lv_obj_center(label);

  // --- Zone de contenu ---
  lv_obj_t *contenu = lv_obj_create(ecran);
  lv_obj_set_width(contenu, lv_pct(100));
  lv_obj_set_flex_grow(contenu, 1);
  lv_obj_set_style_radius(contenu, 0, 0);
  lv_obj_set_style_border_width(contenu, 0, 0);
  lv_obj_set_style_bg_opa(contenu, LV_OPA_TRANSP, 0);

  // Marges : de l'air autour, et de la place à droite pour la barre de défilement
  lv_obj_set_style_pad_top(contenu, 10, 0);
  lv_obj_set_style_pad_bottom(contenu, 16, 0);
  lv_obj_set_style_pad_left(contenu, 8, 0);
  lv_obj_set_style_pad_right(contenu, 12, 0);
  lv_obj_set_style_pad_row(contenu, 8, 0);

  // Barre de défilement fine, visible seulement pendant le défilement
  lv_obj_set_scrollbar_mode(contenu, LV_SCROLLBAR_MODE_AUTO);   // Visible dès que le contenu dépasse
  lv_obj_set_style_width(contenu, 3, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_right(contenu, 3, LV_PART_SCROLLBAR);

  lv_obj_set_flex_flow(contenu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(contenu, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  *ecranOut = ecran;
  return contenu;
}

// ---------- Barre d'appui (calque supérieur) ----------
static void creerBarreAppui() {
  barreAppui = lv_bar_create(lv_layer_top());
  lv_obj_set_size(barreAppui, lv_pct(100), 6);
  lv_obj_align(barreAppui, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, LV_PART_INDICATOR);
  lv_bar_set_range(barreAppui, 0, LONG_PRESS_INTERVAL);
  lv_group_remove_obj(barreAppui);

  lv_obj_t *repere = lv_obj_create(barreAppui);
  lv_obj_set_size(repere, 2, lv_pct(100));
  lv_obj_set_x(repere, lv_pct(SHORT_PRESS_INTERVAL * 100 / LONG_PRESS_INTERVAL));
  lv_obj_set_style_bg_color(repere, lv_color_white(), 0);
  lv_obj_set_style_border_width(repere, 0, 0);
  lv_obj_set_style_radius(repere, 0, 0);
  lv_obj_set_style_pad_all(repere, 0, 0);
  lv_obj_set_scrollable(repere, false);

  lv_obj_set_hidden(barreAppui, true);
}

static void majBarreAppui() {
  static bool visible = false;

  if (!lvglBoutonPresse()) {
    if (visible) { lv_obj_set_hidden(barreAppui, true); visible = false; }
    return;
  }

  uint32_t duree = millis() - lvglDebutAppui();

  lv_color_t c = duree < SHORT_PRESS_INTERVAL ? lv_palette_main(LV_PALETTE_GREEN)
               : duree < LONG_PRESS_INTERVAL  ? lv_palette_main(LV_PALETTE_ORANGE)
               :                                lv_palette_main(LV_PALETTE_RED);
  lv_obj_set_style_bg_color(barreAppui, c, LV_PART_INDICATOR);

  if (duree > LONG_PRESS_INTERVAL) duree = LONG_PRESS_INTERVAL;
  lv_bar_set_value(barreAppui, duree, LV_ANIM_OFF);

  if (!visible) { lv_obj_set_hidden(barreAppui, false); visible = true; }
}

// ---------- Transitions ----------
// En paysage, les glissements font ressortir le tearing : on passe en fondu
static bool paysage() {
  lv_display_t *d = lv_display_get_default();
  return lv_display_get_horizontal_resolution(d) > lv_display_get_vertical_resolution(d);
}

static lv_screen_load_anim_t animEntree() {
  return paysage() ? LV_SCR_LOAD_ANIM_FADE_IN : LV_SCR_LOAD_ANIM_MOVE_LEFT;
}

static lv_screen_load_anim_t animSortie() {
  return paysage() ? LV_SCR_LOAD_ANIM_FADE_IN : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
}

// ---------- Menu principal ----------
static void clicMenuCb(lv_event_t *e) {
  ouvrirApp((int8_t)(intptr_t)lv_event_get_user_data(e), animEntree());
}

static void afficherMenu(lv_screen_load_anim_t anim) {
  lv_group_remove_all_objs(lv_group_get_default());

  lv_obj_t *ecran;
  lv_obj_t *contenu = creerEcran("SilkyOS", &ecran);

  lv_obj_t *aFocus = nullptr;

  for (uint8_t i = 0; i < NB_APPS; i++) {
    // Carte cliquable : [pastille] Nom ........ >
    lv_obj_t *btn = lv_button_create(contenu);
    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
    themeCarte(btn);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn, 10, 0);

    // Pastille colorée avec l'icône
    lv_obj_t *pastille = lv_obj_create(btn);
    lv_obj_set_size(pastille, 30, 30);
    lv_obj_set_style_radius(pastille, 8, 0);
    lv_obj_set_style_bg_color(pastille, lv_color_hex(apps[i].couleur), 0);
    lv_obj_set_style_border_width(pastille, 0, 0);
    lv_obj_set_style_pad_all(pastille, 0, 0);
    lv_obj_set_scrollable(pastille, false);

    lv_obj_t *icone = lv_label_create(pastille);
    lv_label_set_text(icone, apps[i].icone);
    lv_obj_set_style_text_color(icone, lv_color_white(), 0);
    lv_obj_center(icone);

    // Nom : prend toute la place, ce qui pousse le chevron à droite
    lv_obj_t *nom = lv_label_create(btn);
    lv_label_set_text(nom, apps[i].nom);
    lv_obj_set_flex_grow(nom, 1);

    lv_obj_t *chevron = lv_label_create(btn);
    lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(chevron, lv_color_hex(COUL_TEXTE_2), 0);

    lv_obj_add_event_cb(btn, clicMenuCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    if (i == selection) aFocus = btn;
  }

  activerDefilement(contenu);          // Après la boucle : les boutons sont dans le groupe

  if (aFocus) lv_group_focus_obj(aFocus);

  lv_screen_load_anim(ecran, anim, 200, 0, true);
  appActive = -1;
}

// ---------- Apps et sous-pages ----------
// Note : onCreate d'une app est rappelée quand on revient d'une de ses sous-pages
static void ouvrirApp(int8_t index, lv_screen_load_anim_t anim) {
  selection = index;
  appActive = index;
  lv_group_remove_all_objs(lv_group_get_default());

  lv_obj_t *ecran;
  lv_obj_t *contenu = creerEcran(apps[index].nom, &ecran);
  apps[index].onCreate(contenu);
  activerDefilement(contenu);

  lv_screen_load_anim(ecran, anim, 200, 0, true);
}

void uiOuvrirPage(const char *titre, void (*onCreate)(lv_obj_t *), void (*onExit)()) {
  dansPage = true;
  pageExit = onExit;
  lv_group_remove_all_objs(lv_group_get_default());

  lv_obj_t *ecran;
  lv_obj_t *contenu = creerEcran(titre, &ecran);
  onCreate(contenu);
  activerDefilement(contenu);

  lv_screen_load_anim(ecran, animEntree(), 200, 0, true);
}

// Nettoie la sous-page (timers...) sans changer d'écran
static void quitterPage() {
  if (pageExit) pageExit();
  pageExit = nullptr;
  dansPage = false;
}

// Moyen depuis une sous-page : on reconstruit l'app parente
static void fermerPage() {
  quitterPage();
  ouvrirApp(appActive, animSortie());
}

static void fermerApp() {
  if (appActive < 0) return;
  if (dansPage) quitterPage();
  if (apps[appActive].onExit) apps[appActive].onExit();
  afficherMenu(animSortie());
}

// ---------- API publique ----------
void uiInit() {
  creerBarreAppui();
  afficherMenu(LV_SCR_LOAD_ANIM_NONE);
}

void uiUpdate() {
  majBarreAppui();
  majDefilement();

  // Reconstruction demandée (changement de thème, reset...)
  if (rechargementDemande) {
    rechargementDemande = false;
    if (appActive < 0)  afficherMenu(LV_SCR_LOAD_ANIM_FADE_IN);
    else if (!dansPage) ouvrirApp(appActive, LV_SCR_LOAD_ANIM_FADE_IN);
  }

  bool retour = lvglPopRetour();
  bool menu   = lvglPopMenu();

  if (appActive < 0) return;          // Déjà dans le menu

  if (menu) {
    fermerApp();                      // Long : retour direct au menu
  } else if (retour) {
    if (dansPage) fermerPage();       // Moyen : remonte d'un niveau
    else          fermerApp();
  }
}