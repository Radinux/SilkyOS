#ifndef LV_CONF_H
#define LV_CONF_H

// ---------- Mémoire de LVGL ----------
// Allocateur intégré de LVGL, mais son bloc de travail est demandé en PSRAM
// au démarrage, au lieu d'être un tableau fixe en RAM interne.
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_BUILTIN
#define LV_MEM_SIZE           (256 * 1024U)          // 4x plus grand, et en PSRAM
#define LV_MEM_ADR            0
#define LV_MEM_POOL_INCLUDE   <esp_heap_caps.h>
#define LV_MEM_POOL_ALLOC(taille)  heap_caps_malloc((taille), MALLOC_CAP_SPIRAM)

// ---------- Polices embarquées ----------
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_USE_LOG            0


// ---------- Cadence ----------
// 16 ms au lieu de 33 : l'encodeur est lu, les animations avancent et l'écran se redessine
// jusqu'à 60 fois par seconde (si le processeur suit)
#define LV_DEF_REFR_PERIOD  16

// ---------- Accélérations ----------
// Les ombres de même forme (le halo des tuiles) ne sont calculées qu'une fois, puis réutilisées
#define LV_DRAW_SW_SHADOW_CACHE_SIZE  64


#endif