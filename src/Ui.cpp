#include <Arduino.h>
#include <lvgl.h>
#include "App.h"
#include "Lvgl.h"
#include "Ui.h"
#include "Button.h"     // Pour SHORT_PRESS_INTERVAL et LONG_PRESS_INTERVAL

static int8_t appActive = -1;    // -1 = on est dans le menu
static int8_t selection = 0;     // Dernière app choisie (pour restaurer le focus)

static lv_obj_t *barreAppui = nullptr;

// Barre d'appui sur le calque supérieur : visible par-dessus tous les écrans
static void creerBarreAppui() {
  barreAppui = lv_bar_create(lv_layer_top());
  lv_obj_set_size(barreAppui, lv_pct(100), 6);
  lv_obj_align(barreAppui, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, 0);
  lv_obj_set_style_radius(barreAppui, 0, LV_PART_INDICATOR);
  lv_bar_set_range(barreAppui, 0, LONG_PRESS_INTERVAL);
  lv_group_remove_obj(barreAppui);            // Jamais focusable par l'encodeur

  // Repère blanc au seuil "moyen", positionné en pourcentage
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

// Appelée à chaque loop : suit la durée de l'appui en cours
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

static void ouvrirApp(int8_t index);

// Crée un écran avec barre de titre. Renvoie la zone de contenu à remplir.
static lv_obj_t *creerEcran(const char *titre, lv_obj_t **ecranOut) {
  lv_obj_t *ecran = lv_obj_create(nullptr);
  lv_obj_set_flex_flow(ecran, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(ecran, 0, 0);
  lv_obj_set_style_pad_gap(ecran, 0, 0);

  // --- En-tête ---
  lv_obj_t *entete = lv_obj_create(ecran);
  lv_obj_set_size(entete, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_radius(entete, 0, 0);
  lv_obj_set_style_border_width(entete, 0, 0);
  lv_obj_set_style_pad_ver(entete, 6, 0);
  lv_obj_set_scrollable(entete, false);

  lv_obj_t *label = lv_label_create(entete);
  lv_label_set_text(label, titre);
  lv_obj_center(label);

  // --- Contenu : prend toute la hauteur restante ---
  lv_obj_t *contenu = lv_obj_create(ecran);
  lv_obj_set_width(contenu, lv_pct(100));
  lv_obj_set_flex_grow(contenu, 1);
  lv_obj_set_style_radius(contenu, 0, 0);
  lv_obj_set_style_border_width(contenu, 0, 0);
  lv_obj_set_style_bg_opa(contenu, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(contenu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(contenu, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  *ecranOut = ecran;
  return contenu;
}

// ---------- Menu principal ----------
static void clicMenuCb(lv_event_t *e) {
  ouvrirApp((int8_t)(intptr_t)lv_event_get_user_data(e));
}

static void afficherMenu(lv_screen_load_anim_t anim) {
  lv_group_remove_all_objs(lv_group_get_default());   // Le focus repart de zéro

  lv_obj_t *ecran;
  lv_obj_t *contenu = creerEcran("ATS-OS", &ecran);
  lv_obj_set_style_pad_row(contenu, 8, 0);

  lv_obj_t *aFocus = nullptr;

  for (uint8_t i = 0; i < NB_APPS; i++) {
    lv_obj_t *btn = lv_button_create(contenu);
    lv_obj_set_size(btn, lv_pct(90), 60);
    lv_obj_set_style_bg_color(btn, lv_color_hex(apps[i].couleur), 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text_fmt(label, "%s  %s", apps[i].icone, apps[i].nom);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, clicMenuCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    if (i == selection) aFocus = btn;
  }

  if (aFocus) lv_group_focus_obj(aFocus);    // On revient sur l'app qu'on vient de quitter

  lv_screen_load_anim(ecran, anim, 200, 0, true);   // true = supprime l'ancien écran
  appActive = -1;
}

// ---------- Ouverture / fermeture d'app ----------
static void ouvrirApp(int8_t index) {
  selection = index;
  lv_group_remove_all_objs(lv_group_get_default());

  lv_obj_t *ecran;
  lv_obj_t *contenu = creerEcran(apps[index].nom, &ecran);
  apps[index].onCreate(contenu);             // L'app construit ses widgets

  lv_screen_load_anim(ecran, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
  appActive = index;
}

static void fermerApp() {
  if (appActive < 0) return;
  if (apps[appActive].onExit) apps[appActive].onExit();
  afficherMenu(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}

// ---------- API publique ----------
void uiInit() {
  creerBarreAppui();                       // ← ajout, une seule fois au démarrage
  afficherMenu(LV_SCR_LOAD_ANIM_NONE);
}

void uiUpdate() {
  majBarreAppui();                         // ← ajout

  bool retour = lvglPopRetour();
  bool menu   = lvglPopMenu();

  if (appActive >= 0 && (retour || menu)) fermerApp();
}