#include <string.h>
#include "Theme.h"
#include "Storage.h"

// ---------- Palettes ----------
// Même base sombre pour toutes, seul l'accent change.
// ⚠️ On ajoute toujours les nouvelles palettes À LA FIN : le thème sauvegardé est un numéro,
// insérer au milieu décalerait le thème de tout le monde.
const Palette palettes[] = {
  //  nom         fond      carte     focus     texte     texte2    accent    vert      rouge
  { "Bleu",     0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x0A84FF, 0x30D158, 0xFF453A },
  { "Orange",   0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFF9F0A, 0x30D158, 0xFF453A },
  { "Menthe",   0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x66D4CF, 0x30D158, 0xFF453A },
  { "Violet",   0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xBF5AF2, 0x30D158, 0xFF453A },
  { "Rose",     0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFF375F, 0x30D158, 0xFF453A },
  { "Rouge",    0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFF453A, 0x30D158, 0xFF453A },
  { "Jaune",    0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFFD60A, 0x30D158, 0xFF453A },
  { "Vert",     0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x30D158, 0x30D158, 0xFF453A },
  { "Indigo",   0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x5E5CE6, 0x30D158, 0xFF453A },
  { "Cyan",     0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x64D2FF, 0x30D158, 0xFF453A },
  { "Graphite", 0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xAEAEB2, 0x30D158, 0xFF453A },
};

const uint8_t  NB_PALETTES   = sizeof(palettes) / sizeof(palettes[0]);
const Palette *paletteActive = &palettes[0];

// Force du fondu d'accent en bas des cartes (0 = aucun, 255 = accent pur)
static const uint8_t FONDU_REPOS = 60;     
static const uint8_t FONDU_FOCUS = 80;     // ~27 % : la carte sélectionnée ressort

static lv_style_t styleCarte;
static lv_style_t styleCarteFocus;

// Mélange l'accent dans une couleur de carte
static lv_color_t fondu(uint32_t carte, uint8_t force) {
  return lv_color_mix(lv_color_hex(COUL_ACCENT), lv_color_hex(carte), force);
}

lv_color_t themeFonduCarte() { return fondu(COUL_CARTE, FONDU_REPOS); }

// Recolore nos styles et prévient LVGL que les objets qui les utilisent doivent se redessiner
static void majStyles() {
  // Carte : couleur habituelle en haut, légèrement teintée d'accent en bas
  lv_style_set_bg_color(&styleCarte, lv_color_hex(COUL_CARTE));
  lv_style_set_bg_grad_color(&styleCarte, fondu(COUL_CARTE, FONDU_REPOS));
  lv_style_set_text_color(&styleCarte, lv_color_hex(COUL_TEXTE));

  // Carte sélectionnée : un cran plus claire, et un fondu plus marqué
  lv_style_set_bg_color(&styleCarteFocus, lv_color_hex(COUL_CARTE_FOCUS));
  lv_style_set_bg_grad_color(&styleCarteFocus, fondu(COUL_CARTE_FOCUS, FONDU_FOCUS));
  lv_style_set_border_color(&styleCarteFocus, lv_color_hex(COUL_ACCENT));

  lv_obj_report_style_change(&styleCarte);
  lv_obj_report_style_change(&styleCarteFocus);
}

void themeInit() {
  // Carte : gris très foncé, bien arrondie, sans ombre, avec un fondu vertical
  lv_style_init(&styleCarte);
  lv_style_set_bg_opa(&styleCarte, LV_OPA_COVER);
  lv_style_set_bg_grad_dir(&styleCarte, LV_GRAD_DIR_VER);   // bg_color en haut → grad_color en bas
  lv_style_set_radius(&styleCarte, 14);
  lv_style_set_shadow_width(&styleCarte, 0);
  lv_style_set_border_width(&styleCarte, 2);             // Bordure TOUJOURS présente...
  lv_style_set_border_opa(&styleCarte, LV_OPA_TRANSP);   // ...mais invisible au repos
  lv_style_set_pad_hor(&styleCarte, 8);                  // 10 - 2 : compense la bordure
  lv_style_set_pad_ver(&styleCarte, 8);

  // Carte sélectionnée : un cran plus claire, la bordure devient visible (même épaisseur)
  lv_style_init(&styleCarteFocus);
  lv_style_set_border_opa(&styleCarteFocus, LV_OPA_COVER);
  lv_style_set_outline_width(&styleCarteFocus, 0);       // Pas le contour du thème

  themeAppliquer(reglages.theme);
}

void themeAppliquer(uint8_t index) {
  if (index >= NB_PALETTES) index = 0;
  paletteActive = &palettes[index];

  majStyles();

  // Thème de base LVGL (sliders, dropdowns, focus...) recoloré
  lv_display_t *disp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(disp,
      lv_color_hex(COUL_ACCENT),
      lv_color_hex(COUL_VERT),
      true,                            // Mode sombre
      LV_FONT_DEFAULT);
  lv_display_set_theme(disp, theme);
}

void themeCarte(lv_obj_t *obj) {
  lv_obj_add_style(obj, &styleCarte, 0);
  // À l'encodeur, un widget est FOCUSED *et* FOCUS_KEY, et FOCUS_KEY l'emporte :
  // on applique notre style aux deux, sinon le contour du thème revient par FOCUS_KEY
  lv_obj_add_style(obj, &styleCarteFocus, LV_STATE_FOCUSED);
  lv_obj_add_style(obj, &styleCarteFocus, LV_STATE_FOCUS_KEY);
}

const char *themeOptions() {
  static char options[192] = "";      // 11 noms et plus : de la marge
  if (options[0] == '\0') {            // Construit une seule fois
    for (uint8_t i = 0; i < NB_PALETTES; i++) {
      strcat(options, palettes[i].nom);
      if (i < NB_PALETTES - 1) strcat(options, "\n");
    }
  }
  return options;
}