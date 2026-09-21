#include <Arduino.h>
#include <WiFi.h>
#include "../App.h"
#include "../Network.h"

static lv_obj_t   *labelStatut, *labelSsid, *labelIp, *labelRssi, *barreSignal;
static lv_timer_t *timerWifi = nullptr;

// Petit titre gris + label de valeur en dessous
static lv_obj_t *creerChamp(lv_obj_t *parent, const char *titre) {
  lv_obj_t *t = lv_label_create(parent);
  lv_label_set_text(t, titre);
  lv_obj_set_style_text_color(t, lv_palette_main(LV_PALETTE_GREY), 0);
  return lv_label_create(parent);
}

static void majWifi(lv_timer_t *) {
  bool ok = netIsConnected();

  lv_label_set_text(labelStatut, netGetStatusText());
  lv_obj_set_style_text_color(labelStatut,
      ok ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_ORANGE), 0);

  lv_label_set_text(labelSsid, netGetSSID().c_str());
  lv_label_set_text(labelIp,   netGetIP().c_str());

  if (ok) {
    int rssi = WiFi.RSSI();
    lv_bar_set_value(barreSignal, constrain(rssi, -90, -40), LV_ANIM_ON);
    lv_label_set_text_fmt(labelRssi, "%d dBm", rssi);
    lv_obj_set_hidden(barreSignal, false);
  } else {
    lv_label_set_text(labelRssi, "-");
    lv_obj_set_hidden(barreSignal, true);
  }
}

void wifiPageCreate(lv_obj_t *contenu) {
  lv_obj_set_style_pad_row(contenu, 4, 0);

  labelStatut = lv_label_create(contenu);
  lv_obj_set_style_text_font(labelStatut, &lv_font_montserrat_20, 0);

  labelSsid = creerChamp(contenu, "Reseau");
  labelIp   = creerChamp(contenu, "Adresse IP");
  labelRssi = creerChamp(contenu, "Signal");

  barreSignal = lv_bar_create(contenu);
  lv_obj_set_width(barreSignal, lv_pct(90));
  lv_bar_set_range(barreSignal, -90, -40);

  majWifi(nullptr);
  timerWifi = lv_timer_create(majWifi, 1000, nullptr);
}

void wifiPageExit() {
  if (timerWifi) { lv_timer_delete(timerWifi); timerWifi = nullptr; }
}