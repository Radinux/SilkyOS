#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>

struct Theme {
  uint16_t fond, entete, texte, accent1, accent2, accent3;
};

extern LGFX_Sprite spr;
extern const Theme TH;

void displayInit();
void displayPush();          // Envoie le sprite à l'écran
void displaySetBrightness(uint8_t niveau);    // ← ajout (0-255)

#endif