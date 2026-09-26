#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

struct Reglages {
  int     compteur;
  uint8_t luminosite;
  bool    sensEncodeur;
  uint8_t modeWifi;        // 0=OFF, 1=AP, 2=Box
  uint8_t rotation;      // 0=0°, 1=90°, 2=180°, 3=270° (relatif à ROTATION_BASE)
  uint8_t theme;         // Index de la palette de couleurs
  uint8_t fuseau;        // Index du fuseau (0 = UTC-12 ... 12 = UTC+0 ... 26 = UTC+14)
  uint8_t veille;        // Index de la durée de veille (voir uiVeilleOptions)
};

extern Reglages reglages;

void storageInit();
void storageSave();
void storageReset();


#endif