#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <SI4735.h>
#include "Config.h"
#include "Radio.h"

static SI4735   rx;
static bool     allumee   = false;
static bool     charge    = false;
static uint16_t frequence = 10000;       // 100,0 MHz par défaut
static uint8_t  volume    = 30;
static char     nomStation[9] = "";      // Le nom RDS fait 8 caractères au maximum

// Dernière fréquence et dernier volume, dans l'espace NVS propre à la radio
static void charger() {
  if (charge) return;
  Preferences p;
  p.begin("radio", true);
  frequence = p.getUShort("freq", 10000);
  volume    = p.getUChar("vol", 30);
  p.end();
  charge = true;
}

void radioSauver() {
  // Appelée en quittant l'app, pas à chaque cran : on ménage la flash
  Preferences p;
  p.begin("radio", false);
  p.putUShort("freq", frequence);
  p.putUChar("vol", volume);
  p.end();
}

// ---------- Marche / arrêt ----------
bool radioAllumer() {
  if (allumee) return true;
  charger();

  // 1. Ampli coupé pendant tout le démarrage : pas de "clac" dans les écouteurs
  pinMode(AMP_EN_PIN, OUTPUT);
  digitalWrite(AMP_EN_PIN, LOW);

  // 2. Bus I2C, puis on cherche la puce (son adresse dépend du câblage : 0x11 ou 0x63)
  Wire.begin(SI_I2C_SDA, SI_I2C_SCL);
  if (!rx.getDeviceI2CAddress(SI_RESET_PIN)) {
    Serial.println("[Radio] SI4732 introuvable");
    return false;
  }

  // 3. Démarrage en FM
  rx.setup(SI_RESET_PIN, 0);                    // 0 = démarrage en FM (1 = AM)
  rx.setAudioMuteMcuPin(AUDIO_MUTE_PIN);        // La bibliothèque gère le mute matériel
  rx.setFM(8750, 10800, frequence, 10);         // 87,5 à 108 MHz, pas de 100 kHz
  rx.setFMDeEmphasis(1);                        // 50 µs : la norme européenne
  rx.RdsInit();
  rx.setRdsConfig(1, 2, 2, 2, 2);               // RDS actif, tolérance d'erreurs moyenne
  rx.setVolume(volume);
  rx.setHardwareAudioMute(false);

  // 4. L'ampli en DERNIER, une fois tout le reste en place
  delay(50);
  digitalWrite(AMP_EN_PIN, HIGH);

  nomStation[0] = '\0';
  allumee = true;
  Serial.printf("[Radio] Allumee sur %u.%u MHz\n", frequence / 100, (frequence / 10) % 10);
  return true;
}

void radioEteindre() {
  if (!allumee) return;
  digitalWrite(AMP_EN_PIN, LOW);                // L'ampli en PREMIER : extinction silencieuse
  rx.setHardwareAudioMute(true);
  rx.powerDown();                               // La puce ne consomme presque plus rien
  allumee = false;
  Serial.println("[Radio] Eteinte");
}

bool radioEstAllumee() { return allumee; }

// ---------- Réglages ----------
void radioSetFrequence(uint16_t f) {
  charger();
  if (f == frequence) return;
  frequence = f;
  nomStation[0] = '\0';                         // Nouvelle station : l'ancien nom ne vaut plus
  if (allumee) {
    rx.setFrequency(f);                         // Bloque ~30 ms le temps de l'accord
    rx.RdsInit();                               // On vide les restes RDS de l'ancienne station
  }
}

uint16_t radioFrequence() { charger(); return frequence; }

void radioSetVolume(uint8_t v) {
  charger();
  volume = v > 63 ? 63 : v;
  if (allumee) rx.setVolume(volume);
}

uint8_t radioVolume() { charger(); return volume; }

// ---------- Mesures ----------
void radioSignal(uint8_t *rssi, uint8_t *snr) {
  if (!allumee) { *rssi = *snr = 0; return; }
  rx.getCurrentReceivedSignalQuality();
  *rssi = rx.getCurrentRSSI();
  *snr  = rx.getCurrentSNR();
}

void radioTick() {
  if (!allumee) return;
  rx.getRdsStatus();
  if (!(rx.getRdsReceived() && rx.getRdsSync() && rx.getRdsSyncFound())) return;

  const char *nom = rx.getRdsStationName();
  if (!nom) return;

  // On ne garde que des caractères affichables (le RDS reçu peut être bruité)
  uint8_t n = 0;
  for (uint8_t i = 0; i < 8 && nom[i]; i++) {
    char c = nom[i];
    nomStation[n++] = (c >= ' ' && c <= '~') ? c : ' ';
  }
  nomStation[n] = '\0';
}

const char *radioNomStation() { return nomStation; }