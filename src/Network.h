#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

enum ModeWifi { ATS_WIFI_OFF = 0, ATS_WIFI_AP = 1, ATS_WIFI_STA = 2 };

void        netInit();                  // Crée la tâche réseau (cœur 0) + applique le mode sauvegardé
void        netApply(uint8_t mode);     // DEMANDE un changement de mode : ne bloque jamais
bool        netIsConnected();
String      netGetIP();
String      netGetSSID();
const char *netGetStatusText();

#endif