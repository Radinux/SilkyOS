#include <Preferences.h>
#include "Storage.h"

static Preferences prefs;
static const char *NAMESPACE = "ats-os";

Reglages reglages = { 0, 200, true, 0 };     // Ajout du 0 final

void storageInit() {
  prefs.begin(NAMESPACE, false);
  reglages.compteur     = prefs.getInt("compteur", 0);
  reglages.luminosite   = prefs.getUChar("lumi", 200);
  reglages.sensEncodeur = prefs.getBool("sens", true);
  reglages.modeWifi     = prefs.getUChar("wifi", 0);     // ← ajout
  Serial.println("[OK] Reglages charges");
}

void storageSave() {
  prefs.putInt("compteur", reglages.compteur);
  prefs.putUChar("lumi",   reglages.luminosite);
  prefs.putBool("sens",    reglages.sensEncodeur);
  prefs.putUChar("wifi",   reglages.modeWifi);           // ← ajout
  Serial.println("[OK] Reglages sauvegardes");
}

void storageReset() {
  prefs.clear();
  reglages = { 0, 200, true, 0 };                        // ← ajout
  Serial.println("[OK] Reglages remis a zero");
}