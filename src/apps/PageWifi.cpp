#include <Arduino.h>
#include <WiFi.h>
#include "../App.h"
#include "../Network.h"
#include "../Theme.h"
#include "../Widgets.h"

static lv_obj_t   *icone, *labelStatut, *labelSsid, *labelIp, *barreSignal, *labelRssi;
static lv_timer_t *timerWifi = nullptr;

static void majWifi(lv_timer_t *) {
  bool ok = netIsConnected();
  lv_color_t c = lv_color_hex(ok ? COUL_VERT : COUL_ATTENTION);

  // Carte héros : icône + statut colorés
  lv_obj_set_style_text_color(icone, c, 0);
  lv_obj_set_style_text_color(labelStatut, c, 0);
  lv_label_set_text(labelStatut, netGetStatusText());

  lv_label_set_text(labelSsid, netGetSSID().c_str());
  lv_label_set_text(labelIp,   netGetIP().c_str());

  if (ok) {
    int rssi = WiFi.RSSI();
    lv_bar_set_value(barreSignal, constrain(rssi, -90, -40), LV_ANIM_ON);
    lv_label_set_text_fmt(labelRssi, "%d dBm", rssi);
  } else {
    lv_bar_set_value(barreSignal, -90, LV_ANIM_ON);
    lv_label_set_text(labelRssi, "-");
  }
}

void wifiPageCreate(lv_obj_t *contenu) {
  // --- Carte héros ---
  lv_obj_t *heros = creerCarte(contenu);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  icone = lv_label_create(heros);
  lv_label_set_text(icone, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(icone, &lv_font_montserrat_28, 0);

  labelStatut = lv_label_create(heros);
  lv_obj_set_style_text_font(labelStatut, &lv_font_montserrat_20, 0);

  // --- Détails ---
  labelSsid   = creerInfo(contenu, "Reseau");
  labelIp     = creerInfo(contenu, "Adresse IP");
  barreSignal = creerJauge(contenu, "Signal", &labelRssi);
  lv_bar_set_range(barreSignal, -90, -40);

  majWifi(nullptr);
  timerWifi = lv_timer_create(majWifi, 1000, nullptr);
}

void wifiPageExit() {
  if (timerWifi) { lv_timer_delete(timerWifi); timerWifi = nullptr; }
}