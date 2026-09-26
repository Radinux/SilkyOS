#include <Arduino.h>
#include "../App.h"
#include "../Clock.h"
#include "../Meteo.h"
#include "../Network.h"
#include "../Theme.h"
#include "../Widgets.h"

static lv_obj_t   *labelVille, *labelTemp, *labelDesc, *labelDetails, *labelPrevi, *labelEtat;
static lv_timer_t *timerMeteo  = nullptr;
static uint32_t    derniereMaj = UINT32_MAX;     // Données déjà affichées (majMillis)

static const char *jours[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };

static void majMeteo(lv_timer_t *) {
  Meteo m;
  bool ok = meteoGet(&m);                        // Copie protégée par le mutex

  // --- Ligne d'état : ce qui manque, ou l'âge des données ---
  if (!meteoVilleDefinie()) {
    lv_label_set_text_fmt(labelEtat, "Choisis une ville sur\nhttp://%s", netGetIP().c_str());
  } else if (!ok) {
    lv_label_set_text(labelEtat, netIsConnected() ? "Telechargement..." : "En attente du WiFi");
  } else {
    lv_label_set_text_fmt(labelEtat, "Mis a jour il y a %lu min",
                          (unsigned long)((millis() - m.majMillis) / 60000));
  }

  lv_label_set_text(labelVille, m.ville[0] ? m.ville : "Meteo");

  if (!ok) {
    if (derniereMaj != UINT32_MAX) {             // On efface une seule fois
      lv_label_set_text(labelTemp, "--");
      lv_label_set_text(labelDesc, "");
      lv_label_set_text(labelDetails, "");
      lv_label_set_text(labelPrevi, "-");
      derniereMaj = UINT32_MAX;
    }
    return;
  }

  if (m.majMillis == derniereMaj) return;        // Rien de neuf depuis le dernier affichage
  derniereMaj = m.majMillis;

  lv_label_set_text_fmt(labelTemp, "%d", m.temp);
  lv_label_set_text(labelDesc, meteoDescription(m.code));
  lv_label_set_text_fmt(labelDetails, "Humidite %u %%\nVent %d km/h", m.humidite, m.vent);

  // --- Prévisions sur 3 jours ---
  struct tm t;
  bool heure = clockGet(&t);
  char texte[128] = "";
  for (int i = 0; i < 3; i++) {
    const char *nom = (i == 0) ? "Auj." : (i == 1) ? "Demain"
                    : (heure ? jours[(t.tm_wday + 2) % 7] : "J+2");
    char ligne[40];
    snprintf(ligne, sizeof(ligne), "%s%s   %d / %d C", i ? "\n" : "", nom, m.tmin[i], m.tmax[i]);
    strcat(texte, ligne);
  }
  lv_label_set_text(labelPrevi, texte);
}

static void actualiserCb(lv_event_t *) {
  meteoDemanderMaj();                            // La tâche réseau s'en chargera
  lv_label_set_text(labelEtat, "Actualisation...");
}

void meteoCreate(lv_obj_t *contenu) {
  lv_obj_t *gauche, *droite;
  creerColonnes(contenu, &gauche, &droite);

  // --- Gauche : la météo actuelle ---
  lv_obj_t *heros = creerCarte(gauche);
  lv_obj_set_flex_align(heros, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(heros, 2, 0);

  labelVille = lv_label_create(heros);
  lv_obj_set_style_text_color(labelVille, lv_color_hex(COUL_TEXTE_2), 0);

  // La température en grand, avec le "C" en petit et en haut, façon exposant
  lv_obj_t *rangeeTemp = creerRangeeVide(heros);
  lv_obj_set_style_pad_column(rangeeTemp, 2, 0);
  lv_obj_set_flex_align(rangeeTemp, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  labelTemp = lv_label_create(rangeeTemp);
  lv_label_set_text(labelTemp, "--");
  lv_obj_set_style_text_font(labelTemp, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(labelTemp, lv_color_hex(COUL_TEXTE), 0);

  lv_obj_t *unite = lv_label_create(rangeeTemp);
  lv_label_set_text(unite, "C");
  lv_obj_set_style_text_font(unite, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(unite, lv_color_hex(COUL_ACCENT), 0);

  labelDesc = lv_label_create(heros);
  lv_obj_set_style_text_color(labelDesc, lv_color_hex(COUL_ACCENT), 0);

  labelDetails = lv_label_create(heros);
  lv_obj_set_style_text_font(labelDetails, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(labelDetails, lv_color_hex(COUL_TEXTE_2), 0);
  lv_obj_set_style_text_align(labelDetails, LV_TEXT_ALIGN_CENTER, 0);

  // --- Droite : prévisions, actualisation, état ---
  labelPrevi = creerInfo(droite, "Previsions");

  lv_obj_t *rangee = creerRangeeVide(droite);
  creerBoutonAction(rangee, LV_SYMBOL_REFRESH, COUL_ACCENT, actualiserCb);

  labelEtat = lv_label_create(droite);
  lv_obj_set_width(labelEtat, lv_pct(100));
  lv_obj_set_style_text_font(labelEtat, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(labelEtat, lv_color_hex(COUL_TEXTE_2), 0);
  lv_obj_set_style_text_align(labelEtat, LV_TEXT_ALIGN_CENTER, 0);

  derniereMaj = 0;                               // Force le premier affichage
  majMeteo(nullptr);
  timerMeteo = lv_timer_create(majMeteo, 1000, nullptr);
}

void meteoExit() {
  if (timerMeteo) { lv_timer_delete(timerMeteo); timerMeteo = nullptr; }
}