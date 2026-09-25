#include "Battery.h"

// ---------- Matériel (repris du firmware ATS Mini) ----------
#define VBAT_MON         4        // GPIO4 = ADC1 : compatible avec le WiFi
#define BATT_ADC_READS   10       // Moyenne sur 10 lectures
#define BATT_ADC_FACTOR  1.702f   // Brut → mV, pont diviseur inclus (calibré par l'équipe ATS)
#define SEUIL_USB        4.30f    // Au-delà, on lit l'USB (~4,65 V), pas la batterie

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

static float    volts       = 0;    // Tension lissée
static uint32_t derniereMes = 0;

static float mesurer() {
  uint32_t somme = 0;
  for (int i = 0; i < BATT_ADC_READS; i++) somme += analogRead(VBAT_MON);
  return (float)somme / BATT_ADC_READS * BATT_ADC_FACTOR / 1000.0f;
}

void batteryInit() {
  volts = mesurer();                // Première mesure, sans lissage
  derniereMes = millis();
}

void batteryTick() {
  if (millis() - derniereMes < 1000) return;
  derniereMes = millis();

  float v = mesurer();

  if (v > SEUIL_USB || volts > SEUIL_USB) {
    volts = v;                      // Branchement / débranchement : on suit immédiatement
  } else {
    volts = volts * 0.9f + v * 0.1f;   // Lissage exponentiel : 10 % de nouveau à chaque mesure
  }
}

float batteryVolts() { return volts; }

bool batteryOnUsb() { return volts > SEUIL_USB; }

int batteryPercent() {
  if (volts <= courbe[0].volts)             return 0;
  if (volts >= courbe[NB_POINTS - 1].volts) return 100;

  // Interpolation linéaire entre les deux points qui encadrent la tension
  for (int i = 1; i < NB_POINTS; i++) {
    if (volts < courbe[i].volts) {
      const PointCourbe &a = courbe[i - 1];
      const PointCourbe &b = courbe[i];
      return a.pct + (int)((volts - a.volts) * (b.pct - a.pct) / (b.volts - a.volts));
    }
  }
  return 100;
}