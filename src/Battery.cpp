#include "Battery.h"

// ---------- Matériel (repris du firmware ATS Mini) ----------
#define VBAT_MON         4        // GPIO4 = ADC1 : compatible avec le WiFi
#define BATT_ADC_FACTOR  1.702f   // Brut → mV, pont diviseur inclus (calibré par l'équipe ATS)
#define SEUIL_USB        4.30f    // Au-delà, on lit l'USB (~4,65 V), pas la batterie

// ---------- Réglages de la mesure ----------
#define NB_ECHANTILLONS  32       // Moyenne sur 32 lectures pour gommer le bruit de l'ADC
#define PERIODE_MS       200      // 5 mesures par seconde
#define ALPHA            0.2f     // Lissage : 20 % de nouveau à chaque mesure (~1 s de constante de temps)

#define STABILISATION_MS 5000     // Après débranchement USB : attente avant de figer un %
#define BAISSE_CONFIRMEE 25       // Baisse maintenue 25 mesures (5 s) avant de descendre d'1 %
#define HAUSSE_FRANCHE   5        // On ne remonte que pour +5 % d'un coup (après une recharge...)

// ---------- Courbe de décharge LiPo (V → %) ----------
// Calée sur les seuils ATS : 3,68 / 3,78 / 3,88 V = 25 / 50 / 75 %
struct PointCourbe { float volts; int pct; };

static const PointCourbe courbe[] = {
  { 3.30f,   0 },
  { 3.50f,  10 },
  { 3.68f,  25 },
  { 3.78f,  50 },
  { 3.88f,  75 },
  { 4.05f,  95 },
  { 4.15f, 100 },
};
static const int NB_POINTS = sizeof(courbe) / sizeof(courbe[0]);

// ---------- État ----------
static float    volts        = 0;     // Tension lissée
static uint32_t derniereMes  = 0;
static int      pctAffiche   = -1;    // -1 = pas encore figé
static uint32_t finUsb       = 0;     // Instant du débranchement USB
static int      compteBaisse = 0;

// ---------- Outils ----------
static float mesurer() {
  uint32_t somme = 0;
  for (int i = 0; i < NB_ECHANTILLONS; i++) somme += analogRead(VBAT_MON);
  return (float)somme / NB_ECHANTILLONS * BATT_ADC_FACTOR / 1000.0f;
}

// Interpolation linéaire sur la courbe de décharge
static int pourcentageDe(float v) {
  if (v <= courbe[0].volts)             return 0;
  if (v >= courbe[NB_POINTS - 1].volts) return 100;

  for (int i = 1; i < NB_POINTS; i++) {
    if (v < courbe[i].volts) {
      const PointCourbe &a = courbe[i - 1];
      const PointCourbe &b = courbe[i];
      return a.pct + (int)((v - a.volts) * (b.pct - a.pct) / (b.volts - a.volts));
    }
  }
  return 100;
}

// ---------- API ----------
void batteryInit() {
  volts = mesurer();
  derniereMes = millis();
  if (volts <= SEUIL_USB) pctAffiche = pourcentageDe(volts);   // Au boot : valeur directe
}

void batteryTick() {
  if (millis() - derniereMes < PERIODE_MS) return;
  derniereMes = millis();

  float v = mesurer();
  bool usbAvant = (volts > SEUIL_USB);
  bool usbMaint = (v > SEUIL_USB);

  if (usbAvant != usbMaint) {
    // Branchement ou débranchement : on suit immédiatement
    volts = v;
    if (!usbMaint) {                 // On vient de débrancher
      finUsb = millis();
      pctAffiche = -1;               // On attendra que la tension se stabilise
    }
    compteBaisse = 0;
    return;
  }

  volts += ALPHA * (v - volts);      // Lissage exponentiel
  if (usbMaint) return;              // Sur USB, le niveau batterie n'est pas lisible

  int mesure = pourcentageDe(volts);

  // Pas encore de valeur figée : on attend la fin de la stabilisation
  if (pctAffiche < 0) {
    if (millis() - finUsb >= STABILISATION_MS) pctAffiche = mesure;
    return;
  }

  // Remontée : seulement si elle est franche (évite le yoyo quand la charge diminue)
  if (mesure >= pctAffiche + HAUSSE_FRANCHE) {
    pctAffiche = mesure;
    compteBaisse = 0;
    return;
  }

  // Baisse : seulement si elle dure (un pic de WiFi ne suffit pas)
  if (mesure < pctAffiche) {
    if (++compteBaisse >= BAISSE_CONFIRMEE) {
      pctAffiche--;                  // Une marche à la fois, en douceur
      compteBaisse = 0;
    }
  } else {
    compteBaisse = 0;
  }
}

float batteryVolts() { return volts; }

bool batteryOnUsb() { return volts > SEUIL_USB; }

int batteryPercent() {
  // Pendant la stabilisation, on donne quand même une estimation
  return pctAffiche >= 0 ? pctAffiche : pourcentageDe(volts);
}