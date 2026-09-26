#include <string.h>
#include "Theme.h"
#include "Storage.h"

// ---------- Palettes ----------
// Même base sombre pour toutes, seul l'accent change.
// Rien n'empêche plus tard d'ajouter des palettes aux fonds différents.
const Palette palettes[] = {
  //  nom        fond      carte     focus     texte     texte2    accent    vert      rouge
  { "Bleu",    0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x0A84FF, 0x30D158, 0xFF453A },
  { "Orange",  0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFF9F0A, 0x30D158, 0xFF453A },
  { "Menthe",  0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0x66D4CF, 0x30D158, 0xFF453A },
  { "Violet",  0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xBF5AF2, 0x30D158, 0xFF453A },
  { "Rose",    0x000000, 0x1C1C1E, 0x2C2C2E, 0xFFFFFF, 0x8E8E93, 0xFF375F, 0x30D158, 0xFF453A },
};

const uint8_t  NB_PALETTES   = sizeof(palettes) / sizeof(palettes[0]);
const Palette *paletteActive = &palettes[0];

static lv_style_t styleCarte;
static lv_style_t styleCarteFocus;

// Recolore nos styles et prévient LVGL que les objets qui les utilisent doivent se redessiner
static void majStyles() {
  lv_style_set_bg_color(&styleCarte, lv_color_hex(COUL_CARTE));
  lv_style_set_text_color(&styleCarte, lv_color_hex(COUL_TEXTE));

  lv_style_set_bg_color(&styleCarteFocus, lv_color_hex(COUL_CARTE_FOCUS));
  lv_style_set_border_color(&styleCarteFocus, lv_color_hex(COUL_ACCENT));

  lv_obj_report_style_change(&styleCarte);
  lv_obj_report_style_change(&styleCarteFocus);
}

void themeInit() {
  // Propriétés fixes, communes à toutes les palettes
  lv_style_init(&styleCarte);
  lv_style_set_bg_opa(&styleCarte, LV_OPA_COVER);
  lv_style_set_radius(&styleCarte, 14);
  lv_style_set_border_width(&styleCarte, 0);
  lv_style_set_shadow_width(&styleCarte, 0);
  lv_style_set_pad_hor(&styleCarte, 10);
  lv_style_set_pad_ver(&styleCarte, 10);

  lv_style_init(&styleCarteFocus);
  lv_style_set_border_width(&styleCarteFocus, 2);
  lv_style_set_outline_width(&styleCarteFocus, 0);

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
  static char options[128] = "";
  if (options[0] == '\0') {            // Construit une seule fois
    for (uint8_t i = 0; i < NB_PALETTES; i++) {
      strcat(options, palettes[i].nom);
      if (i < NB_PALETTES - 1) strcat(options, "\n");
    }
  }
  return options;
}