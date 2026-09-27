#include <Arduino.h>
#include <lvgl.h>
#include "../Battery.h"
#include "../Clock.h"
#include "../Display.h"
#include "../Icones.h"
#include "../Lvgl.h"
#include "../Meteo.h"
#include "../Network.h"
#include "../Storage.h"
#include "../Theme.h"
#include "../Ui.h"
#include "UiInterne.h"

// Durées en secondes, dans le même ordre que uiVeilleOptions() (0 = jamais)
static const uint16_t DUREES[]   = { 0, 15, 30, 60, 120, 300 };
static const uint8_t  NB_DUREES  = sizeof(DUREES) / sizeof(DUREES[0]);

static const uint8_t  LUMI_AOD   = 38;      // ~15 % de 255
static const uint32_t CPU_VEILLE = 80;      // MHz : le minimum compatible avec le WiFi
static const uint32_t CPU_NORMAL = 240;

// Fenêtre de synchronisation WiFi pendant la veille
static const uint32_t PERIODE_FENETRE = 60UL * 60 * 1000;   // Une fois par heure
static const uint32_t DUREE_FENETRE   = 60UL * 1000;        // Au plus 1 minute

static bool      enVeille = false;
static lv_obj_t *voileAod = nullptr, *heureAod, *dateAod, *battAod;
static lv_obj_t *bandeMeteo, *icoMeteoAod, *tempMeteoAod;
static int       derniereMinuteAod = -2;

static bool      wifiCoupe      = false;    // Le WiFi a-t-il été coupé par la veille ?
static bool      fenetreActive  = false;    // WiFi rallumé temporairement pour se synchroniser
static uint32_t  debutCoupure   = 0;
static uint32_t  debutFenetre   = 0;
static uint32_t  meteoAvant     = 0;        // Date de la météo au début de la fenêtre

const char *uiVeilleOptions() {
  return "Jamais\n15 s\n30 s\n1 min\n2 min\n5 min";
}

bool uiEnVeille() { return enVeille; }

// ---------- AOD : une horloge minimaliste sur le calque supérieur ----------
static void creerAod() {
  voileAod = uiCreerVoile();

  heureAod = lv_label_create(voileAod);
  lv_obj_set_style_text_font(heureAod, &lv_font_montserrat_48, 0);

  dateAod = lv_label_create(voileAod);

  // Bande météo : icône colorée + température, côte à côte
  bandeMeteo = lv_obj_create(voileAod);
  lv_obj_set_size(bandeMeteo, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(bandeMeteo, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bandeMeteo, 0, 0);
  lv_obj_set_style_pad_all(bandeMeteo, 0, 0);
  lv_obj_set_style_pad_column(bandeMeteo, 6, 0);
  lv_obj_set_scrollable(bandeMeteo, false);
  lv_obj_set_flex_flow(bandeMeteo, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bandeMeteo, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  icoMeteoAod = lv_label_create(bandeMeteo);
  lv_obj_set_style_text_font(icoMeteoAod, &silky_icones_20, 0);
  tempMeteoAod = lv_label_create(bandeMeteo);

  battAod = lv_label_create(voileAod);
  lv_obj_set_style_text_font(battAod, &lv_font_montserrat_12, 0);

  lv_obj_set_hidden(voileAod, true);
}

// Ne redessine que quand la minute change : ~1 rafraîchissement par minute
static void majAod() {
  static const char *jours[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };
  struct tm t;
  int minute = clockGet(&t) ? t.tm_hour * 60 + t.tm_min : -1;
  if (minute == derniereMinuteAod) return;
  derniereMinuteAod = minute;

  if (minute < 0) {
    lv_label_set_text(heureAod, "--:--");
    lv_label_set_text(dateAod, "");
  } else {
    lv_label_set_text_fmt(heureAod, "%02d:%02d", t.tm_hour, t.tm_min);
    lv_label_set_text_fmt(dateAod, "%s %d", jours[t.tm_wday], t.tm_mday);
  }

  // Bande météo : affichée seulement si on a des données
  Meteo m;
  if (meteoGet(&m)) {
    const char *ico;
    uint32_t couleur;
    iconeMeteo(m.code, &ico, &couleur);
    lv_label_set_text(icoMeteoAod, ico);
    lv_obj_set_style_text_color(icoMeteoAod, lv_color_hex(couleur), 0);
    lv_label_set_text_fmt(tempMeteoAod, "%d C", m.temp);
    lv_obj_set_hidden(bandeMeteo, false);
  } else {
    lv_obj_set_hidden(bandeMeteo, true);
  }

  if (batteryOnUsb()) lv_label_set_text(battAod, LV_SYMBOL_CHARGE);
  else                lv_label_set_text_fmt(battAod, "%d%%", batteryPercent());
}

// ---------- WiFi en veille : coupé, avec une courte fenêtre de synchro par heure ----------
static void gererFenetreWifi() {
  if (!wifiCoupe) return;
  uint32_t maintenant = millis();
  Meteo m;

  if (!fenetreActive) {
    if (maintenant - debutCoupure < PERIODE_FENETRE) return;

    // C'est l'heure : on rallume le WiFi (l'heure NTP se recale à la connexion)
    meteoGet(&m);
    meteoAvant = m.majMillis;
    netApply(reglages.modeWifi);
    meteoDemanderMaj();                     // Sera traité dès que la connexion est établie
    fenetreActive = true;
    debutFenetre  = maintenant;
    Serial.println("[Veille] Fenetre WiFi ouverte");
    return;
  }

  // Fenêtre ouverte : on referme dès que la météo est arrivée, ou au bout du délai maximal
  bool meteoRecue = meteoGet(&m) && m.majMillis != meteoAvant;
  if (meteoRecue || maintenant - debutFenetre > DUREE_FENETRE) {
    netApply(ATS_WIFI_OFF);
    fenetreActive = false;
    debutCoupure  = maintenant;
    derniereMinuteAod = -2;                 // L'AOD affichera la nouvelle météo tout de suite
    Serial.printf("[Veille] Fenetre WiFi fermee (%s)\n", meteoRecue ? "meteo recue" : "delai");
  }
}

// ---------- Entrée et sortie de veille ----------
static void entrerVeille() {
  enVeille = true;
  lvglSetVeille(true);

  if (reglages.aod) {
    if (!voileAod) creerAod();

    // Couleurs du thème actif, posées à chaque entrée (le thème a pu changer)
    lv_obj_set_style_bg_color(voileAod, lv_color_hex(COUL_FOND), 0);
    lv_obj_set_style_text_color(heureAod, lv_color_hex(COUL_TEXTE), 0);
    lv_obj_set_style_text_color(dateAod,  lv_color_hex(COUL_ACCENT), 0);
    lv_obj_set_style_text_color(tempMeteoAod, lv_color_hex(COUL_TEXTE_2), 0);
    lv_obj_set_style_text_color(battAod,  lv_color_hex(COUL_TEXTE_2), 0);

    derniereMinuteAod = -2;                 // Force l'affichage immédiat
    majAod();
    lv_obj_set_hidden(voileAod, false);
    displaySetBrightness(LUMI_AOD);
  } else {
    displayEteindre();
  }

  setCpuFrequencyMhz(CPU_VEILLE);           // Moins vite = beaucoup moins de courant

  // Le WiFi est le plus gros consommateur : on le coupe pendant la veille
  if (reglages.modeWifi != ATS_WIFI_OFF) {
    netApply(ATS_WIFI_OFF);
    wifiCoupe     = true;
    fenetreActive = false;
    debutCoupure  = millis();
  }
}

void veilleReveiller() {
  if (!enVeille) return;
  enVeille = false;
  lvglSetVeille(false);

  setCpuFrequencyMhz(CPU_NORMAL);

  if (wifiCoupe) {
    netApply(reglages.modeWifi);            // Reconnexion en tâche de fond : l'écran n'attend pas
    wifiCoupe     = false;
    fenetreActive = false;
  }

  if (voileAod) lv_obj_set_hidden(voileAod, true);
  displaySetBrightness(reglages.luminosite);
}

// autorisee = false pendant une mise à jour ou une alerte : on reste (ou on se remet) allumé
void veilleMaj(bool autorisee) {
  if (enVeille) {
    if (lvglPopReveil() || !autorisee) { veilleReveiller(); return; }
    if (reglages.aod) majAod();
    gererFenetreWifi();
    return;
  }

  uint16_t duree = DUREES[reglages.veille < NB_DUREES ? reglages.veille : 0];
  if (!autorisee || duree == 0) return;

  if (lvglInactivite() > duree * 1000UL) entrerVeille();
}