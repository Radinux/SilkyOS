#include <Arduino.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include "Rollback.h"

static const uint8_t  MAX_ESSAIS       = 3;        // Démarrages ratés avant le retour en arrière
static const uint32_t DELAI_VALIDATION = 30000;    // 30 s sans planter = firmware validé

static bool enEssai       = false;
static bool retourArriere = false;

void rollbackVerifier() {
  Preferences p;
  p.begin("rollback", false);

  enEssai       = p.getBool("essai", false);
  retourArriere = p.getBool("retour", false);
  if (retourArriere) p.putBool("retour", false);   // On ne l'annonce qu'une seule fois

  if (enEssai) {
    uint8_t n = p.getUChar("n", 0) + 1;
    p.putUChar("n", n);
    Serial.printf("[Rollback] Demarrage d'essai %u/%u\n", n, MAX_ESSAIS);

    if (n >= MAX_ESSAIS) {
      // Trop d'échecs : on bascule sur l'autre emplacement, celui de l'ancien firmware
      const esp_partition_t *ancien = esp_ota_get_next_update_partition(nullptr);
      p.putBool("essai", false);
      p.putUChar("n", 0);

      if (ancien && esp_ota_set_boot_partition(ancien) == ESP_OK) {   // Vérifie aussi l'image
        p.putBool("retour", true);
        p.end();
        Serial.println("[Rollback] Retour a l'ancien firmware !");
        delay(100);
        esp_restart();
      }

      // Pas d'ancien firmware valide : rien de mieux à faire que de continuer
      Serial.println("[Rollback] Aucun ancien firmware valide, on continue");
      enEssai = false;
    }
  }
  p.end();
}

void rollbackArmer() {
  Preferences p;
  p.begin("rollback", false);
  p.putBool("essai", true);
  p.putUChar("n", 0);
  p.end();
  Serial.println("[Rollback] Prochain demarrage a l'essai");
}

void rollbackTick() {
  if (!enEssai || millis() < DELAI_VALIDATION) return;

  Preferences p;
  p.begin("rollback", false);
  p.putBool("essai", false);
  p.putUChar("n", 0);
  p.end();
  enEssai = false;
  Serial.println("[Rollback] Nouveau firmware valide !");
}

bool rollbackEffectue() { return retourArriere; }