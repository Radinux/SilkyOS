#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

struct Reglages {
  int     compteur;
  uint8_t luminosite;
  bool    sensEncodeur;
  uint8_t modeWifi;        // 0=OFF, 1=AP, 2=Box
};

extern Reglages reglages;

void storageInit();
void storageSave();
void storageReset();

#endif