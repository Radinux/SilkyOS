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

enum EtatOta { OTA_AUCUN = 0, OTA_EN_COURS, OTA_REUSSI, OTA_ECHEC };

uint8_t netOtaEtat();         // Où en est la mise à jour
uint8_t netOtaPourcent();     // Progression estimée (0-100)
void    netOtaAcquitter();    // L'UI a affiché l'échec : on revient à OTA_AUCUN

#endif