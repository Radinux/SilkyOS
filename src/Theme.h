#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

struct Palette {
  const char *nom;
  uint32_t fond, carte, carteFocus, texte, texte2, accent, vert, rouge;
};

extern const Palette  palettes[];
extern const uint8_t  NB_PALETTES;
extern const Palette *paletteActive;

// Les couleurs lisent la palette active : tout le code existant suit automatiquement
#define COUL_FOND         (paletteActive->fond)
#define COUL_CARTE        (paletteActive->carte)
#define COUL_CARTE_FOCUS  (paletteActive->carteFocus)
#define COUL_TEXTE        (paletteActive->texte)
#define COUL_TEXTE_2      (paletteActive->texte2)
#define COUL_ACCENT       (paletteActive->accent)
#define COUL_VERT         (paletteActive->vert)
#define COUL_ROUGE        (paletteActive->rouge)
#define COUL_ATTENTION    0xFF9F0A   // Orange : avertissement (fixe, quel que soit le thème)

void        themeInit();                    // Styles + palette sauvegardée
void        themeAppliquer(uint8_t index);  // Change de palette
void        themeCarte(lv_obj_t *obj);      // Style carte + surbrillance au focus
const char *themeOptions();                 // "Bleu\nOrange\n..." pour un dropdown
lv_color_t  themeFonduCarte();              // Couleur du bas d'une carte au repos (fondu d'accent)

#endif