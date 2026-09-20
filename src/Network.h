#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

// Préfixe ATS_ pour éviter toute collision avec les constantes de l'ESP-IDF
enum ModeWifi { ATS_WIFI_OFF = 0, ATS_WIFI_AP = 1, ATS_WIFI_STA = 2 };

void        netInit();
void        netApply(uint8_t mode);
void        netTick();                  // ← celui qui manquait
bool        netIsConnected();
String      netGetIP();
String      netGetSSID();
const char *netGetStatusText();

#endif