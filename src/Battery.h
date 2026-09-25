#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

void  batteryInit();
void  batteryTick();          // À appeler dans loop() : mesure une fois par seconde
float batteryVolts();         // Tension lissée (V)
int   batteryPercent();       // Estimation 0-100 %
bool  batteryOnUsb();         // Alimenté par l'USB : le niveau batterie n'est plus lisible

#endif