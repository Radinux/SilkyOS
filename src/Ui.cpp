#include <Arduino.h>
#include <lvgl.h>
#include "App.h"
#include "Button.h"
#include "Lvgl.h"
#include "Theme.h"
#include "Ui.h"
#include "Config.h"
#include "Network.h"
#include "Battery.h"

static int8_t appActive = -1;        // -1 = menu principal
static int8_t selection = 0;         // Dernière app ouverte (focus au retour)

// Sous-page ouverte par l'app active (un seul niveau)
static bool  dansPage     = false;
static void (*pageExit)() = nullptr;

static lv_obj_t *barreAppui    = nullptr;
static lv_obj_t *labelBatterie = nullptr;    // ← déplacée ici

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
  lv_obj_set_style_pad_top(entete, 20, 0);
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

  lv_obj_set_hidden(labelBatterie, false);     // Pas sur le boot screen, mais partout ensuite
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

// ---------- Indicateur batterie (calque supérieur) ----------

static void creerIndicateurBatterie() {
  labelBatterie = lv_label_create(lv_layer_top());
  lv_obj_set_style_text_font(labelBatterie, &lv_font_montserrat_12, 0);
  lv_obj_align(labelBatterie, LV_ALIGN_TOP_RIGHT, -8, 4);   // Barre d'état, tout en haut
  lv_obj_set_hidden(labelBatterie, true);                   // Visible à partir du menu
}

// Ne redessine que quand l'affichage change
static void majIndicateurBatterie() {
  static int dernierAffiche = -2;              // -1 = USB

  int affiche = batteryOnUsb() ? -1 : batteryPercent();
  if (affiche == dernierAffiche) return;
  dernierAffiche = affiche;

  if (affiche < 0) {
    // Sur USB, le niveau n'est pas lisible : juste l'éclair
    lv_label_set_text(labelBatterie, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(labelBatterie, lv_color_hex(COUL_VERT), 0);
    return;
  }

  static const char *symboles[] = {
    LV_SYMBOL_BATTERY_EMPTY, LV_SYMBOL_BATTERY_1, LV_SYMBOL_BATTERY_2,
    LV_SYMBOL_BATTERY_3, LV_SYMBOL_BATTERY_FULL,
  };
  int niveau = affiche >= 90 ? 4 : affiche >= 65 ? 3 : affiche >= 40 ? 2 : affiche >= 15 ? 1 : 0;

  lv_label_set_text_fmt(labelBatterie, "%d%% %s", affiche, symboles[niveau]);
  lv_obj_set_style_text_color(labelBatterie,
      lv_color_hex(niveau == 0 ? COUL_ROUGE : COUL_TEXTE_2), 0);
}

// ---------- Écran de démarrage ----------
static void finDemarrageCb(lv_timer_t *) {
  afficherMenu(LV_SCR_LOAD_ANIM_FADE_IN);     // Supprime l'écran de boot au passage
}

static void afficherDemarrage() {
  lv_obj_t *ecran = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(ecran, lv_color_hex(COUL_FOND), 0);
  lv_obj_set_flex_flow(ecran, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(ecran, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(ecran, paysage() ? 8 : 14, 0);   // Plus serré en paysage

  // Logo : "Silky" en blanc + "OS" en couleur d'accent
  lv_obj_t *logo = lv_obj_create(ecran);
  lv_obj_set_size(logo, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(logo, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(logo, 0, 0);
  lv_obj_set_style_pad_all(logo, 0, 0);
  lv_obj_set_scrollable(logo, false);
  lv_obj_set_flex_flow(logo, LV_FLEX_FLOW_ROW);

  lv_obj_t *silky = lv_label_create(logo);
  lv_label_set_text(silky, "Silky");
  lv_obj_set_style_text_font(silky, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(silky, lv_color_hex(COUL_TEXTE), 0);

  lv_obj_t *os = lv_label_create(logo);
  lv_label_set_text(os, "OS");
  lv_obj_set_style_text_font(os, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(os, lv_color_hex(COUL_ACCENT), 0);

  // Version, discrète
  lv_obj_t *version = lv_label_create(ecran);
  lv_label_set_text(version, "v" SILKY_VERSION);
  lv_obj_set_style_text_color(version, lv_color_hex(COUL_TEXTE_2), 0);

  // Spinner fin, aux couleurs du thème
  if (paysage()) {
    // Paysage : peu de hauteur, une fine barre qui se remplit pendant le démarrage
    lv_obj_t *barre = lv_bar_create(ecran);
    lv_obj_set_size(barre, lv_pct(50), 4);
    lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_CARTE_FOCUS), 0);
    lv_obj_set_style_bg_color(barre, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_anim_duration(barre, 1600, 0);   // Durée du remplissage animé
    lv_bar_set_value(barre, 100, LV_ANIM_ON);
  } else {
    // Portrait : le spinner fin
    lv_obj_t *spinner = lv_spinner_create(ecran);
    lv_obj_set_size(spinner, 30, 30);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COUL_CARTE_FOCUS), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COUL_ACCENT), LV_PART_INDICATOR);
    lv_group_remove_obj(spinner);
  }

  lv_obj_fade_in(logo, 600, 0);             // Le logo apparaît en douceur

  lv_screen_load_anim(ecran, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);

  // Timer "one-shot" : se déclenche une fois, puis se supprime tout seul
  lv_timer_t *t = lv_timer_create(finDemarrageCb, 1800, nullptr);
  lv_timer_set_repeat_count(t, 1);
}

// ---------- Écran de mise à jour (calque supérieur) ----------
static lv_obj_t *voileMaj, *arcMaj, *labelPctMaj, *barreMaj, *labelPctBarre, *labelEtatMaj;

static void creerEcranMaj() {
  voileMaj = lv_obj_create(lv_layer_top());
  lv_obj_set_size(voileMaj, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(voileMaj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(voileMaj, 0, 0);
  lv_obj_set_style_radius(voileMaj, 0, 0);
  lv_obj_set_scrollable(voileMaj, false);
  lv_obj_set_flex_flow(voileMaj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(voileMaj, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *titre = lv_label_create(voileMaj);
  lv_label_set_text(titre, "Mise a jour");
  lv_obj_set_style_text_font(titre, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(titre, lv_color_white(), 0);

  // --- Variante portrait : anneau façon montre, pourcentage au centre ---
  arcMaj = lv_arc_create(voileMaj);
  lv_obj_set_size(arcMaj, 110, 110);
  lv_arc_set_rotation(arcMaj, 270);
  lv_arc_set_bg_angles(arcMaj, 0, 360);
  lv_arc_set_range(arcMaj, 0, 100);
  lv_obj_remove_style(arcMaj, nullptr, LV_PART_KNOB);
  lv_obj_set_style_arc_width(arcMaj, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arcMaj, 10, LV_PART_INDICATOR);
  lv_group_remove_obj(arcMaj);

  labelPctMaj = lv_label_create(arcMaj);
  lv_obj_set_style_text_font(labelPctMaj, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(labelPctMaj, lv_color_white(), 0);
  lv_obj_center(labelPctMaj);

  // --- Variante paysage : fine barre + pourcentage en dessous ---
  barreMaj = lv_bar_create(voileMaj);
  lv_obj_set_size(barreMaj, lv_pct(60), 6);
  lv_bar_set_range(barreMaj, 0, 100);

  labelPctBarre = lv_label_create(voileMaj);
  lv_obj_set_style_text_font(labelPctBarre, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(labelPctBarre, lv_color_white(), 0);

  labelEtatMaj = lv_label_create(voileMaj);

  lv_obj_set_hidden(voileMaj, true);
}

// Appelée à chaque tour : lit l'état écrit par la tâche réseau, et SEULE l'UI touche LVGL
// Appelée à chaque tour : lit l'état écrit par la tâche réseau, et SEULE l'UI touche LVGL
static void majEcranMaj() {
  static bool     visible     = false;
  static uint8_t  dernierEtat = OTA_AUCUN;
  static uint8_t  dernierPct  = 255;
  static uint32_t debutEchec  = 0;

  uint8_t etat = netOtaEtat();

  if (etat == OTA_AUCUN) {
    if (visible) { lv_obj_set_hidden(voileMaj, true); visible = false; }
    dernierEtat = OTA_AUCUN;
    return;
  }

  if (!visible) {
    // Variante choisie selon l'orientation au moment où l'écran apparaît
    bool pay = paysage();
    lv_obj_set_hidden(arcMaj,        pay);
    lv_obj_set_hidden(barreMaj,      !pay);
    lv_obj_set_hidden(labelPctBarre, !pay);
    lv_obj_set_style_pad_row(voileMaj, pay ? 8 : 14, 0);

    // Couleurs du thème actif
    lv_obj_set_style_bg_color(voileMaj, lv_color_hex(COUL_FOND), 0);
    lv_obj_set_style_arc_color(arcMaj, lv_color_hex(COUL_CARTE_FOCUS), LV_PART_MAIN);
    lv_obj_set_style_bg_color(barreMaj, lv_color_hex(COUL_CARTE_FOCUS), 0);

    lv_obj_set_hidden(voileMaj, false);
    visible = true;
    dernierPct = 255;
  }

  // On ne redessine que ce qui change (les deux variantes, c'est plus simple)
  uint8_t p = netOtaPourcent();
  if (p != dernierPct) {
    lv_arc_set_value(arcMaj, p);
    lv_bar_set_value(barreMaj, p, LV_ANIM_ON);
    lv_label_set_text_fmt(labelPctMaj,   "%d%%", p);
    lv_label_set_text_fmt(labelPctBarre, "%d%%", p);
    dernierPct = p;
  }

  if (etat != dernierEtat) {
    dernierEtat = etat;
    uint32_t c;
    switch (etat) {
      case OTA_EN_COURS:
        lv_label_set_text(labelEtatMaj, "Ne pas eteindre");
        c = COUL_ACCENT;
        lv_obj_set_style_text_color(labelEtatMaj, lv_color_hex(COUL_TEXTE_2), 0);
        break;
      case OTA_REUSSI:
        lv_label_set_text(labelEtatMaj, "Redemarrage...");
        c = COUL_VERT;
        lv_obj_set_style_text_color(labelEtatMaj, lv_color_hex(c), 0);
        break;
      default:
        lv_label_set_text(labelEtatMaj, "Echec");
        c = COUL_ROUGE;
        lv_obj_set_style_text_color(labelEtatMaj, lv_color_hex(c), 0);
        debutEchec = millis();
        break;
    }
    lv_obj_set_style_arc_color(arcMaj, lv_color_hex(c), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(barreMaj, lv_color_hex(c), LV_PART_INDICATOR);
  }

  // Après un échec, on laisse le message 3 s puis on rend la main
  if (etat == OTA_ECHEC && millis() - debutEchec > 3000) netOtaAcquitter();
}

// ---------- API publique ----------
void uiInit() {
  creerBarreAppui();
  creerIndicateurBatterie();
  creerEcranMaj();         // Créé en dernier : il passe par-dessus tout
  afficherDemarrage();     // Le menu suivra tout seul, en fondu
}

void uiUpdate() {

  majEcranMaj();
  majIndicateurBatterie();

  // Pendant une mise à jour, on ignore la navigation
  uint8_t ota = netOtaEtat();
  if (ota == OTA_EN_COURS || ota == OTA_REUSSI) return;
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