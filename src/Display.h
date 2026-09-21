#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>

struct Theme {
  uint16_t fond, entete, texte, accent1, accent2, accent3;
};

extern LGFX_Sprite spr;
extern const Theme TH;

int  displayWidth();
int  displayHeight();
void displayFlush(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *px);

void displayInit();
void displayPush();          // Envoie le sprite à l'écran
void displaySetBrightness(uint8_t niveau);    // ← ajout (0-255)

#endif