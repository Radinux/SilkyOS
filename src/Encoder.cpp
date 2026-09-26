#include "Config.h"
#include "Encoder.h"
#include "Storage.h"

static volatile int     compteurBrut = 0;
static volatile uint8_t etat         = 0;

static const uint8_t tableEtats[7][4] = {
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10}, {0x3, 0x2, 0x0, 0x0},
  {0x3, 0x2, 0x1, 0x0}, {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
  {0x6, 0x5, 0x4, 0x0},
};

static void IRAM_ATTR isrEncodeur() {
  uint8_t entrees = (digitalRead(ENCODER_PIN_B) << 1) | digitalRead(ENCODER_PIN_A);
  etat = tableEtats[etat & 0x0F][entrees];

  int8_t sens = reglages.sensEncodeur ? 1 : -1;    // Lit le réglage utilisateur

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
  noInterrupts();               // Section critique : lecture + reset atomiques
  int delta = compteurBrut;
  compteurBrut = 0;
  interrupts();
  return delta;
}

void displayEteindre() {
  ledcWrite(0, 0);          // Vraiment 0, sans le plancher de sécurité de displaySetBrightness()
}