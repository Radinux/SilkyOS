#ifndef METEO_H
#define METEO_H

#include <Arduino.h>

struct Meteo {
  bool     valide;            // Au moins une mise à jour réussie pour cette ville
  char     ville[32];
  int16_t  temp;              // °C, arrondi
  uint8_t  humidite;          // %
  int16_t  vent;              // km/h, arrondi
  uint8_t  code;              // Code météo WMO (ciel dégagé, pluie...)
  int16_t  tmin[3], tmax[3];  // Aujourd'hui, demain, après-demain
  uint8_t  codeJour[3];
  uint32_t majMillis;         // millis() de la dernière mise à jour réussie
};

void        meteoInit();                     // Mutex + ville sauvegardée (avant netInit)
bool        meteoGet(Meteo *copie);          // Copie protégée pour l'UI. false = rien de reçu
bool        meteoVilleDefinie();
void        meteoDefinirVille(const char *nom, float lat, float lon);   // Depuis la page web
void        meteoDemanderMaj();              // Force un rafraîchissement
void        meteoTache();                    // Appelée par la tâche réseau UNIQUEMENT
const char *meteoDescription(uint8_t code);

#endif