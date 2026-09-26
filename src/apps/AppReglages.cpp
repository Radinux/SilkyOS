#include <Arduino.h>
#include "../App.h"
#include "../Ui.h"
#include "../Lvgl.h"
#include "../Display.h"
#include "../Storage.h"
#include "../Network.h"
#include "../Widgets.h"
#include "../Theme.h"

// Sous-pages (définies dans PageWifi.cpp et AppInfos.cpp)
void wifiPageCreate(lv_obj_t *contenu);
void wifiPageExit();
void infosCreate(lv_obj_t *contenu);
void infosExit();

static lv_obj_t *sliderLumi, *swSens, *ddWifi, *ddRotation, *ddTheme, *ddVeille, *ddMenu;
static lv_obj_t *btnWifi, *btnSysteme, *labelReset;
static lv_obj_t *swAod;

// Élément à refocaliser quand l'écran est reconstruit
enum { FOCUS_AUCUN = -1, FOCUS_WIFI, FOCUS_SYSTEME, FOCUS_THEME, FOCUS_ROTATION };
static int8_t focusRetour = FOCUS_AUCUN;

static bool resetArme = false;

static const int LUMI_PAS = 20;    // Slider de 1 à 20 : 20 crans suffisent

// Met les widgets en accord avec les valeurs de "reglages"
static void rafraichirWidgets() {
  int v = reglages.luminosite * LUMI_PAS / 255;
  sliderSetValeur(sliderLumi, v < 1 ? 1 : v);
  lv_obj_set_state(swSens, LV_STATE_CHECKED, reglages.sensEncodeur);
  lv_obj_set_state(swAod, LV_STATE_CHECKED, reglages.aod);
  dropdownSetValeur(ddWifi,     reglages.modeWifi);
  dropdownSetValeur(ddRotation, reglages.rotation);
  dropdownSetValeur(ddTheme,    reglages.theme);
  dropdownSetValeur(ddVeille,   reglages.veille);
  dropdownSetValeur(ddMenu,     reglages.launcher);
}

// ---------- Callbacks ----------
static void lumiCb(lv_event_t *e) {
  reglages.luminosite = lv_slider_get_value(sliderLumi) * 255 / LUMI_PAS;
  displaySetBrightness(reglages.luminosite);     // Effet immédiat, sauvé à la sortie
}

static void sensCb(lv_event_t *e) {
  reglages.sensEncodeur = lv_obj_has_state(swSens, LV_STATE_CHECKED);
  storageSave();
}

static void wifiCb(lv_event_t *e) {
  reglages.modeWifi = lv_dropdown_get_selected(ddWifi);
  storageSave();
  netApply(reglages.modeWifi);
}

static void rotationCb(lv_event_t *e) {
  reglages.rotation = lv_dropdown_get_selected(ddRotation);
  storageSave();
  lvglSetRotation(reglages.rotation);
  focusRetour = FOCUS_ROTATION;    // On revient sur ce réglage après reconstruction
  uiRecharger();                   // L'écran est recréé pour la nouvelle orientation
}

static void themeCb(lv_event_t *e) {
  reglages.theme = lv_dropdown_get_selected(ddTheme);
  storageSave();
  themeAppliquer(reglages.theme);
  focusRetour = FOCUS_THEME;       // On revient sur ce réglage après reconstruction
  uiRecharger();                   // L'écran sera recréé avec les nouvelles couleurs
}

static void veilleCb(lv_event_t *e) {
  reglages.veille = lv_dropdown_get_selected(ddVeille);
  storageSave();
}

static void pageWifiCb(lv_event_t *e) {
  focusRetour = FOCUS_WIFI;
  uiOuvrirPage("Infos WiFi", wifiPageCreate, wifiPageExit);
}

static void pageSystemeCb(lv_event_t *e) {
  focusRetour = FOCUS_SYSTEME;
  uiOuvrirPage("Systeme", infosCreate, infosExit);
}

static void menuCb(lv_event_t *e) {
  reglages.launcher = lv_dropdown_get_selected(ddMenu);
  storageSave();          // Le launcher sera construit avec ce style à son prochain affichage
}

static void aodCb(lv_event_t *e) {
  reglages.aod = lv_obj_has_state(swAod, LV_STATE_CHECKED);
  storageSave();
}

// Double validation : 1er clic arme, 2e clic exécute, quitter le bouton désarme
static void resetCb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_DEFOCUSED) {
    resetArme = false;
    lv_label_set_text(labelReset, "Reinitialiser");
    return;
  }

  if (!resetArme) {
    resetArme = true;
    lv_label_set_text(labelReset, "Confirmer ?");
    return;
  }

  resetArme = false;
  storageReset();
  displaySetBrightness(reglages.luminosite);
  lvglSetRotation(reglages.rotation);
  netApply(reglages.modeWifi);
  themeAppliquer(reglages.theme);
  uiRecharger();                   // Reconstruit l'écran avec les valeurs d'usine
}

// ---------- API de l'app ----------
void reglagesCreate(lv_obj_t *contenu) {
  resetArme = false;

  // Luminosité
  sliderLumi = creerSlider(contenu, "Luminosite", 1, LUMI_PAS, lumiCb);

  // Sens de l'encodeur (interrupteur vert façon iOS)
  lv_obj_t *ligne = creerLigne(contenu, "Enco. CCW");
  swSens = lv_switch_create(ligne);
  lv_obj_set_size(swSens, 40, 22);
  lv_obj_set_style_bg_color(swSens, lv_color_hex(COUL_VERT),
                            LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_event_cb(swSens, sensCb, LV_EVENT_VALUE_CHANGED, nullptr);

  // Listes de choix (composant de Widgets.cpp)
  ddWifi     = creerDropdown(contenu, "WiFi",     "OFF\nAP\nBox",    wifiCb);
  ddRotation = creerDropdown(contenu, "Rotation", "0\n90\n180\n270", rotationCb);
  ddTheme    = creerDropdown(contenu, "Theme",    themeOptions(),    themeCb);
  ddVeille   = creerDropdown(contenu, "Veille",   uiVeilleOptions(), veilleCb);
  lv_obj_t *ligneAod = creerLigne(contenu, "AOD");
  swAod = lv_switch_create(ligneAod);
  lv_obj_set_size(swAod, 40, 22);
  lv_obj_set_style_bg_color(swAod, lv_color_hex(COUL_VERT),
                            LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_event_cb(swAod, aodCb, LV_EVENT_VALUE_CHANGED, nullptr);
  ddMenu     = creerDropdown(contenu, "Menu",     "Liste\nGrille",   menuCb);

  // Sous-pages
  btnWifi    = creerBoutonPage(contenu, "Infos WiFi", pageWifiCb);
  btnSysteme = creerBoutonPage(contenu, "Systeme",    pageSystemeCb);

  // Réinitialisation : carte normale, texte rouge (action destructrice façon iOS)
  lv_obj_t *btnReset = lv_button_create(contenu);
  lv_obj_set_size(btnReset, lv_pct(100), LV_SIZE_CONTENT);
  themeCarte(btnReset);
  labelReset = lv_label_create(btnReset);
  lv_label_set_text(labelReset, "Reinitialiser");
  lv_obj_set_style_text_color(labelReset, lv_color_hex(COUL_ROUGE), 0);
  lv_obj_center(labelReset);
  lv_obj_add_event_cb(btnReset, resetCb, LV_EVENT_CLICKED,   nullptr);
  lv_obj_add_event_cb(btnReset, resetCb, LV_EVENT_DEFOCUSED, nullptr);

  rafraichirWidgets();

  // Écran reconstruit (retour de sous-page, changement de thème) : on remet le focus
  if (focusRetour == FOCUS_WIFI)    lv_group_focus_obj(btnWifi);
  if (focusRetour == FOCUS_SYSTEME) lv_group_focus_obj(btnSysteme);
  if (focusRetour == FOCUS_THEME)   lv_group_focus_obj(ddTheme);
  if (focusRetour == FOCUS_ROTATION) lv_group_focus_obj(ddRotation);
  focusRetour = FOCUS_AUCUN;
}

void reglagesExit() {
  focusRetour = FOCUS_AUCUN;
  storageSave();       // Sauve notamment la luminosité (pas à chaque cran)
}