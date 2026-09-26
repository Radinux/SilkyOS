#include <Preferences.h>
#include "Storage.h"

static Preferences prefs;
static const char *NAMESPACE = "ats-os";

// Défauts : compteur, luminosité, sens, WiFi, rotation, thème, fuseau (UTC+0),
//           veille (1 min), menu (liste), heure d'été (non), AOD (non)
static const Reglages DEFAUTS = { 0, 200, false, 0, 0, 0, 12, 3, 0, false, false };

Reglages reglages = DEFAUTS;

void storageInit() {
  prefs.begin(NAMESPACE, false);

  reglages.compteur     = prefs.getInt("compteur", 0);
  reglages.luminosite   = prefs.getUChar("lumi", 200);
  reglages.sensEncodeur = prefs.getBool("sens", false);
  reglages.modeWifi     = prefs.getUChar("wifi", 0);
  reglages.rotation     = prefs.getUChar("rot", 0);
  reglages.theme        = prefs.getUChar("theme", 0);
  reglages.fuseau       = prefs.getUChar("tz", 12);
  reglages.veille       = prefs.getUChar("veille", 3);
  reglages.launcher     = prefs.getUChar("launch", 0);
  reglages.heureEte     = prefs.getBool("ete", false);
  reglages.aod          = prefs.getBool("aod", false);

  Serial.println("[OK] Reglages charges");
}

void storageSave() {
  prefs.putInt("compteur", reglages.compteur);
  prefs.putUChar("lumi",   reglages.luminosite);
  prefs.putBool("sens",    reglages.sensEncodeur);
  prefs.putUChar("wifi",   reglages.modeWifi);
  prefs.putUChar("rot",    reglages.rotation);
  prefs.putUChar("theme",  reglages.theme);
  prefs.putUChar("tz",     reglages.fuseau);
  prefs.putUChar("veille", reglages.veille);
  prefs.putUChar("launch", reglages.launcher);
  prefs.putBool("ete",     reglages.heureEte);
  prefs.putBool("aod",     reglages.aod);

  Serial.println("[OK] Reglages sauvegardes");
}

void storageReset() {
  prefs.clear();
  reglages = DEFAUTS;
  storageSave();

  Serial.println("[OK] Reglages remis a zero");
}