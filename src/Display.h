#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>

struct Theme {
  uint16_t fond, entete, texte, accent1, accent2, accent3;
};
extern const Theme TH;

void displayInit();
void displaySetBrightness(uint8_t niveau);
void displaySetRotation(uint8_t rotation);   // Rotation relative à ROTATION_BASE
int  displayWidth();
int  displayHeight();
void displayFlush(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *px);

#endif