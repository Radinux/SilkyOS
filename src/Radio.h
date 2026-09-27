#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>

// Pilote du SI4732, en FM uniquement.
// Les fréquences sont en dizaines de kHz, comme dans la bibliothèque : 10390 = 103,9 MHz

bool        radioAllumer();                  // false = puce introuvable sur le bus I2C
void        radioEteindre();
bool        radioEstAllumee();

void        radioSetFrequence(uint16_t f);   // Mémorisée même radio éteinte
uint16_t    radioFrequence();
void        radioSetVolume(uint8_t v);       // 0 à 63
uint8_t     radioVolume();

void        radioSignal(uint8_t *rssi, uint8_t *snr);   // dBµV et dB, 0 si éteinte
const char *radioNomStation();               // Nom RDS, "" si inconnu
void        radioTick();                     // À appeler souvent : lecture du RDS
void        radioSauver();                   // Fréquence et volume en NVS

#endif