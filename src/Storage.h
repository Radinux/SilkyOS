#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

struct Reglages {
  int     compteur;
  uint8_t luminosite;
  bool    sensEncodeur;
};

extern Reglages reglages;

void storageInit();
void storageSave();
void storageReset();

#endif