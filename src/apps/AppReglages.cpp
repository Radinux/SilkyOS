#include <Arduino.h>
#include "../App.h"
#include "../Ui.h"
#include "../Lvgl.h"
#include "../Display.h"
#include "../Storage.h"
#include "../Network.h"
#include "../Widgets.h"

// Sous-pages (définies dans PageWifi.cpp et AppInfos.cpp)
void wifiPageCreate(lv_obj_t *contenu);
void wifiPageExit();
void infosCreate(lv_obj_t *contenu);
void infosExit();

static lv_obj_t *sliderLumi, *swSens, *ddWifi, *ddRotation;
static lv_obj_t *btnWifi, *btnSysteme, *labelReset;

static int8_t dernierePage = -1;   // Sous-page d'où l'on revient (pour le focus)
static bool   resetArme    = false;

static const int LUMI_PAS = 20;    // Slider de 1 à 20 : 20 crans suffisent

// Met les widgets en accord avec les valeurs de "reglages"
static void rafraichirWidgets() {
  int v = reglages.luminosite * LUMI_PAS / 255;
  sliderSetValeur(sliderLumi, v < 1 ? 1 : v);
  lv_obj_set_state(swSens, LV_STATE_CHECKED, reglages.sensEncodeur);
  lv_dropdown_set_selected(ddWifi, reglages.modeWifi);
  lv_dropdown_set_selected(ddRotation, reglages.rotation);
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
}

static void pageWifiCb(lv_event_t *e) {
  dernierePage = 0;
  uiOuvrirPage("Infos WiFi", wifiPageCreate, wifiPageExit);
}

static void pageSystemeCb(lv_event_t *e) {
  dernierePage = 1;
  uiOuvrirPage("Systeme", infosCreate, infosExit);
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
  rafraichirWidgets();
  lv_label_set_text(labelReset, "Fait !");
}

// ---------- API de l'app ----------
void reglagesCreate(lv_obj_t *contenu) {
  resetArme = false;
  lv_obj_set_style_pad_row(contenu, 6, 0);

  // Luminosité : bloc "libellé + valeur + slider" en une ligne
  sliderLumi = creerSlider(contenu, "Luminosite", 1, LUMI_PAS, lumiCb);

  // Sens de l'encodeur
  lv_obj_t *ligne = creerLigne(contenu, "Enco. CCW");
  swSens = lv_switch_create(ligne);
  lv_obj_add_event_cb(swSens, sensCb, LV_EVENT_VALUE_CHANGED, nullptr);

  // Mode WiFi
  ligne = creerLigne(contenu, "WiFi");
  ddWifi = lv_dropdown_create(ligne);
  lv_dropdown_set_options(ddWifi, "OFF\nAP\nBox");
  lv_obj_set_width(ddWifi, 80);
  lv_obj_add_event_cb(ddWifi, wifiCb, LV_EVENT_VALUE_CHANGED, nullptr);

  // Rotation
  ligne = creerLigne(contenu, "Rotation");
  ddRotation = lv_dropdown_create(ligne);
  lv_dropdown_set_options(ddRotation, "0\n90\n180\n270");
  lv_obj_set_width(ddRotation, 80);
  lv_obj_add_event_cb(ddRotation, rotationCb, LV_EVENT_VALUE_CHANGED, nullptr);

  // Sous-pages
  btnWifi    = creerBoutonPage(contenu, "Infos WiFi", pageWifiCb);
  btnSysteme = creerBoutonPage(contenu, "Systeme",    pageSystemeCb);

  // Réinitialisation
  lv_obj_t *btnReset = lv_button_create(contenu);
  lv_obj_set_width(btnReset, lv_pct(100));
  lv_obj_set_style_bg_color(btnReset, lv_palette_main(LV_PALETTE_RED), 0);
  labelReset = lv_label_create(btnReset);
  lv_label_set_text(labelReset, "Reinitialiser");
  lv_obj_center(labelReset);
  lv_obj_add_event_cb(btnReset, resetCb, LV_EVENT_CLICKED,   nullptr);
  lv_obj_add_event_cb(btnReset, resetCb, LV_EVENT_DEFOCUSED, nullptr);

  rafraichirWidgets();

  // Retour d'une sous-page : focus sur le bouton qui l'avait ouverte
  if (dernierePage == 0) lv_group_focus_obj(btnWifi);
  if (dernierePage == 1) lv_group_focus_obj(btnSysteme);
  dernierePage = -1;
}

void reglagesExit() {
  dernierePage = -1;
  storageSave();       // Sauve notamment la luminosité (pas à chaque cran)
}