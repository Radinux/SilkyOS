#include <Arduino.h>
#include <lvgl.h>
#include "App.h"
#include "Lvgl.h"
#include "Theme.h"
#include "Ui.h"
#include "ui/UiInterne.h"

// ================= État de la navigation =================
static int8_t appActive = -1;              // -1 = launcher
static int8_t selection = 0;               // Dernière app ouverte (focus au retour)
static bool   dansPage  = false;           // Une sous-page est ouverte
static void (*pageExit)() = nullptr;
static bool   rechargementDemande = false;

// ================= Défilement à l'encodeur =================
static lv_obj_t     *contenuActif  = nullptr;
static int32_t       cibleScroll   = 0;
static uint32_t      dernierScroll = 0;
static const int32_t PAS_SCROLL    = 40;     // Pixels par cran

static void majDefilement() {
  int32_t d = lvglPopScroll();
  if (d == 0 || contenuActif == nullptr) return;

  int32_t y   = lv_obj_get_scroll_y(contenuActif);
  int32_t max = y + lv_obj_get_scroll_bottom(contenuActif);

  // Crans rapprochés : on prolonge la cible en cours, sinon on repart de la position réelle
  // (le focus a pu faire défiler la page entre-temps)
  int32_t base = (millis() - dernierScroll < 300) ? cibleScroll : y;

  cibleScroll = base + d * PAS_SCROLL;
  if (cibleScroll < 0)   cibleScroll = 0;
  if (cibleScroll > max) cibleScroll = max;
  dernierScroll = millis();

  lv_obj_scroll_to_y(contenuActif, cibleScroll, LV_ANIM_ON);
}

// ================= Mise en page =================
bool uiPaysage() {
  lv_display_t *d = lv_display_get_default();
  return lv_display_get_horizontal_resolution(d) > lv_display_get_vertical_resolution(d);
}

// En paysage, les glissements font ressortir le tearing : on passe en fondu
lv_screen_load_anim_t uiAnimEntree() {
  return uiPaysage() ? LV_SCR_LOAD_ANIM_FADE_IN : LV_SCR_LOAD_ANIM_MOVE_LEFT;
}

lv_screen_load_anim_t uiAnimSortie() {
  return uiPaysage() ? LV_SCR_LOAD_ANIM_FADE_IN : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
}

lv_obj_t *uiCentrer(lv_obj_t *obj) {
  lv_obj_set_scrollable(obj, false);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(obj, uiPaysage() ? 8 : 14, 0);   // Plus serré en paysage
  return obj;
}

lv_obj_t *uiCreerVoile() {
  lv_obj_t *v = uiCentrer(lv_obj_create(lv_layer_top()));
  lv_obj_set_size(v, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(v, lv_color_hex(COUL_FOND), 0);
  lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(v, 0, 0);
  lv_obj_set_style_radius(v, 0, 0);
  return v;
}

lv_obj_t *uiCreerEcran(const char *titre, lv_obj_t **ecranOut) {
  lv_group_remove_all_objs(lv_group_get_default());   // Nouvel écran : le focus repart de zéro
  bool pay = uiPaysage();

  lv_obj_t *ecran = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(ecran, lv_color_hex(COUL_FOND), 0);
  lv_obj_set_flex_flow(ecran, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(ecran, 0, 0);
  lv_obj_set_style_pad_gap(ecran, 0, 0);

  // --- En-tête : sous la barre d'état en portrait, DANS la barre d'état en paysage ---
  lv_obj_t *entete = lv_obj_create(ecran);
  lv_obj_set_size(entete, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(entete, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(entete, 0, 0);
  lv_obj_set_style_pad_top(entete, pay ? 2 : 20, 0);
  lv_obj_set_style_pad_bottom(entete, 2, 0);
  lv_obj_set_scrollable(entete, false);

  lv_obj_t *label = lv_label_create(entete);
  lv_label_set_text(label, titre);
  lv_obj_set_style_text_font(label, pay ? &lv_font_montserrat_14 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COUL_TEXTE), 0);
  lv_obj_center(label);

  // --- Zone de contenu ---
  lv_obj_t *contenu = lv_obj_create(ecran);
  lv_obj_set_width(contenu, lv_pct(100));
  lv_obj_set_flex_grow(contenu, 1);
  lv_obj_set_style_radius(contenu, 0, 0);
  lv_obj_set_style_border_width(contenu, 0, 0);
  lv_obj_set_style_bg_opa(contenu, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_top(contenu, pay ? 4 : 10, 0);
  lv_obj_set_style_pad_bottom(contenu, pay ? 10 : 16, 0);
  lv_obj_set_style_pad_left(contenu, 8, 0);
  lv_obj_set_style_pad_right(contenu, 12, 0);     // Place pour la barre de défilement
  lv_obj_set_style_pad_row(contenu, 8, 0);

  lv_obj_set_scrollbar_mode(contenu, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_width(contenu, 3, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_right(contenu, 3, LV_PART_SCROLLBAR);

  lv_obj_set_flex_flow(contenu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(contenu, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  *ecranOut = ecran;
  return contenu;
}

void uiChargerEcran(lv_obj_t *ecran, lv_obj_t *contenu, lv_screen_load_anim_t anim) {
  contenuActif = contenu;                       // C'est lui que l'encodeur fera défiler
  cibleScroll  = 0;
  lv_screen_load_anim(ecran, anim, 200, 0, true);   // true = supprime l'ancien écran
}

// ================= Navigation =================
void uiAfficherMenu(lv_screen_load_anim_t anim) {
  appActive = -1;
  launcherAfficher(selection, anim);
  barresMontrer();                              // Pas sur le boot screen, partout ensuite
}

// Note : onCreate d'une app est rappelée quand on revient d'une de ses sous-pages
void uiOuvrirApp(int8_t index, lv_screen_load_anim_t anim) {
  selection = appActive = index;

  lv_obj_t *ecran;
  lv_obj_t *contenu = uiCreerEcran(apps[index].nom, &ecran);
  apps[index].onCreate(contenu);
  uiChargerEcran(ecran, contenu, anim);
}

void uiOuvrirPage(const char *titre, void (*onCreate)(lv_obj_t *), void (*onExit)()) {
  dansPage = true;
  pageExit = onExit;

  lv_obj_t *ecran;
  lv_obj_t *contenu = uiCreerEcran(titre, &ecran);
  onCreate(contenu);
  uiChargerEcran(ecran, contenu, uiAnimEntree());
}

// Nettoie la sous-page (timers...) sans changer d'écran
static void quitterPage() {
  if (pageExit) pageExit();
  pageExit = nullptr;
  dansPage = false;
}

static void fermerPage() {                      // Moyen depuis une sous-page
  quitterPage();
  uiOuvrirApp(appActive, uiAnimSortie());
}

static void fermerApp() {
  if (dansPage) quitterPage();
  if (apps[appActive].onExit) apps[appActive].onExit();
  uiAfficherMenu(uiAnimSortie());
}

void uiRecharger() {
  rechargementDemande = true;                   // Traité dans uiUpdate(), hors de tout callback
}

// ================= API =================
void uiInit() {
  barresCreer();
  superpositionsCreer();     // Après les barres : elles passent par-dessus
  demarrageAfficher();       // Le launcher suivra tout seul, en fondu
}

void uiUpdate() {
  superpositionsMaj();
  barresMaj();
  veilleMaj(!superpositionBloque());

  // Travail de fond des apps (minuteur...), qu'elles soient ouvertes ou non
  for (uint8_t i = 0; i < NB_APPS; i++) {
    if (apps[i].onFond) apps[i].onFond();
  }

  // Mise à jour ou alerte à l'écran : navigation suspendue, on jette retour et menu
  if (superpositionBloque()) {
    lvglPopRetour();
    lvglPopMenu();
    return;
  }

  majDefilement();

  if (rechargementDemande) {
    rechargementDemande = false;
    if (appActive < 0)  uiAfficherMenu(LV_SCR_LOAD_ANIM_FADE_IN);
    else if (!dansPage) uiOuvrirApp(appActive, LV_SCR_LOAD_ANIM_FADE_IN);
  }

  bool retour = lvglPopRetour();
  bool menu   = lvglPopMenu();
  if (appActive < 0) return;                    // Déjà dans le launcher

  if (menu || (retour && !dansPage)) fermerApp();    // Long, ou moyen depuis l'app
  else if (retour)                   fermerPage();   // Moyen depuis une sous-page
}