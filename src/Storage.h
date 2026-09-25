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
};

extern Reglages reglages;

void storageInit();
void storageSave();
void storageReset();


#endif