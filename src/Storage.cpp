#include <Preferences.h>
#include "Storage.h"

static Preferences prefs;
static const char *NAMESPACE = "ats-os";

// Valeurs par défaut : compteur, luminosité, sens (false = CW), WiFi, rotation
Reglages reglages = { 0, 200, false, 0, 0 };

void storageInit() {
  prefs.begin(NAMESPACE, false);

  reglages.compteur     = prefs.getInt("compteur", 0);
  reglages.luminosite   = prefs.getUChar("lumi", 200);
  reglages.sensEncodeur = prefs.getBool("sens", false);
  reglages.modeWifi     = prefs.getUChar("wifi", 0);
  reglages.rotation     = prefs.getUChar("rot", 0);

  Serial.println("[OK] Reglages charges");
}

void storageSave() {
  prefs.putInt("compteur", reglages.compteur);
  prefs.putUChar("lumi",   reglages.luminosite);
  prefs.putBool("sens",    reglages.sensEncodeur);
  prefs.putUChar("wifi",   reglages.modeWifi);
  prefs.putUChar("rot",    reglages.rotation);

  Serial.println("[OK] Reglages sauvegardes");
}

void storageReset() {
  prefs.clear();
  reglages = { 0, 200, false, 0, 0 };
  storageSave();                  // On réécrit les défauts pour repartir propre

  Serial.println("[OK] Reglages remis a zero");
}