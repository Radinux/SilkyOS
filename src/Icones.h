#ifndef ICONES_H
#define ICONES_H

#include <lvgl.h>

// Polices d'icônes générées depuis Material Symbols Rounded (Google, licence Apache 2.0),
// version pleine (FILL=1, graisse 500), avec lv_font_conv en 4 bits d'anticrénelage.
LV_FONT_DECLARE(silky_icones_20);   // Liste du launcher (pastilles de 30 px)
LV_FONT_DECLARE(silky_icones_32);   // Grille du launcher, météo (pastilles de 48 px)

// Chaque icône est un caractère Unicode de la zone privée, écrit ici en UTF-8
#define ICO_HORLOGE     "\xEE\xBF\x96"   // schedule (U+EFD6)
#define ICO_CHRONO      "\xEE\x90\xA5"   // timer (U+E425)
#define ICO_MINUTEUR    "\xEE\xA9\x9B"   // hourglass_top (U+EA5B)
#define ICO_JEU         "\xEE\xA8\xA8"   // sports_esports (U+EA28)
#define ICO_METEO       "\xEF\x85\xB2"   // partly_cloudy_day (U+F172)
#define ICO_RADIO       "\xEE\x80\xBE"   // radio (U+E03E)
#define ICO_REGLAGES    "\xEE\xA2\xB8"   // settings (U+E8B8)

// Météo
#define ICO_SOLEIL      "\xEE\xA0\x9A"   // sunny (U+E81A)
#define ICO_NUAGE       "\xEF\x85\x9C"   // cloud (U+F15C)
#define ICO_BROUILLARD  "\xEE\xA0\x98"   // foggy (U+E818)
#define ICO_PLUIE       "\xEF\x85\xB6"   // rainy (U+F176)
#define ICO_NEIGE       "\xEE\x8B\x8D"   // weather_snowy (U+E2CD)
#define ICO_ORAGE       "\xEE\xAF\x9B"   // thunderstorm (U+EBDB)
#define ICO_BRUINE      "\xEE\x8F\xAA"   // grain (U+E3EA)
#define ICO_HUMIDITE    "\xEE\x9E\x98"   // water_drop (U+E798)
#define ICO_VENT        "\xEE\xBF\x98"   // air (U+EFD8)

#endif
