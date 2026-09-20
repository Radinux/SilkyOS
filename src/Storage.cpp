#include <Preferences.h>
#include "Storage.h"

static Preferences prefs;
static const char *NAMESPACE = "ats-os";

Reglages reglages = { 0, 200, true };     // Valeurs par défaut

void storageInit() {
  prefs.begin(NAMESPACE, false);          // false = lecture/écriture

  reglages.compteur     = prefs.getInt("compteur", 0);
  reglages.luminosite   = prefs.getUChar("lumi", 200);
  reglages.sensEncodeur = prefs.getBool("sens", true);

  Serial.println("[OK] Reglages charges");
}

void storageSave() {
  // putX n'écrit que si la valeur a changé : pas d'usure inutile de la flash
  prefs.putInt("compteur", reglages.compteur);
  prefs.putUChar("lumi",   reglages.luminosite);
  prefs.putBool("sens",    reglages.sensEncodeur);

  Serial.println("[OK] Reglages sauvegardes");
}

void storageReset() {
  prefs.clear();
  reglages = { 0, 200, true };
  Serial.println("[OK] Reglages remis a zero");
}