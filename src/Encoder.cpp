#include "Config.h"
#include "Encoder.h"
#include "Storage.h"
#include "soc/gpio_reg.h"      // Adresse du registre d'entrées GPIO
#include "soc/soc.h"           // REG_READ()

static volatile int     compteurBrut = 0;
static volatile uint8_t etat         = 0;

// DRAM_ATTR : la table est placée en RAM interne, lisible même quand le cache flash est coupé
static DRAM_ATTR const uint8_t tableEtats[7][4] = {
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10}, {0x3, 0x2, 0x0, 0x0},
  {0x3, 0x2, 0x1, 0x0}, {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
  {0x6, 0x5, 0x4, 0x0},
};

// Lecture directe du registre matériel, sans digitalRead() (dont le code est en flash).
// Valable pour les GPIO 0 à 31 (nos broches 1 et 2).
static inline uint8_t IRAM_ATTR lirePin(uint8_t pin) {
  return (REG_READ(GPIO_IN_REG) >> pin) & 1;
}

// Interruption : ne touche QUE de l'IRAM et de la DRAM
static void IRAM_ATTR isrEncodeur() {
  uint8_t entrees = (lirePin(ENCODER_PIN_B) << 1) | lirePin(ENCODER_PIN_A);
  etat = tableEtats[etat & 0x0F][entrees];

  int8_t sens = reglages.sensEncodeur ? 1 : -1;   // reglages est en DRAM : lecture sûre

  uint8_t dir = etat & 0x30;
  if (dir == 0x10)      compteurBrut += sens;
  else if (dir == 0x20) compteurBrut -= sens;
}

void encoderInit() {
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), isrEncodeur, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), isrEncodeur, CHANGE);
}

int encoderGetDelta() {
  noInterrupts();               // Section critique : lecture + remise à zéro atomiques
  int delta = compteurBrut;
  compteurBrut = 0;
  interrupts();
  return delta;
}