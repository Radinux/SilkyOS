#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

void encoderInit();
int  encoderGetDelta();   // Renvoie le mouvement depuis le dernier appel, puis remet à 0

#endif