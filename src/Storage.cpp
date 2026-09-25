#include <Preferences.h>
#include "Storage.h"

static Preferences prefs;
static const char *NAMESPACE = "ats-os";

// Défauts : compteur, luminosité, sens (false = CW), WiFi, rotation, thème
Reglages reglages = { 0, 200, false, 0, 0, 0 };

void storageInit() {
  prefs.begin(NAMESPACE, false);

  reglages.compteur     = prefs.getInt("compteur", 0);
  reglages.luminosite   = prefs.getUChar("lumi", 200);
  reglages.sensEncodeur = prefs.getBool("sens", false);
  reglages.modeWifi     = prefs.getUChar("wifi", 0);
  reglages.rotation     = prefs.getUChar("rot", 0);
  reglages.theme        = prefs.getUChar("theme", 0);

  Serial.println("[OK] Reglages charges");
}

void storageSave() {
  prefs.putInt("compteur", reglages.compteur);
  prefs.putUChar("lumi",   reglages.luminosite);
  prefs.putBool("sens",    reglages.sensEncodeur);
  prefs.putUChar("wifi",   reglages.modeWifi);
  prefs.putUChar("rot",    reglages.rotation);
  prefs.putUChar("theme",  reglages.theme);

  Serial.println("[OK] Reglages sauvegardes");
}

void storageReset() {
  prefs.clear();
  reglages = { 0, 200, false, 0, 0, 0 };
  storageSave();

  Serial.println("[OK] Reglages remis a zero");
}