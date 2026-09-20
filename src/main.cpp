#include <Arduino.h>
#include <LovyanGFX.hpp>

#define PIN_POWER_ON         15
#define PIN_LCD_BL           38
#define ENCODER_PIN_A         2
#define ENCODER_PIN_B         1
#define ENCODER_PUSH_BUTTON  21
#define ENCODER_SENS 1   // Mettre 1 si le sens est correct

// ---------- Configuration de l'écran (bus parallèle 8 bits) ----------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;

public:
  LGFX() {
    { // Bus parallèle : 8 lignes de données + contrôle
      auto cfg = _bus.config();
      cfg.freq_write = 20000000;
      cfg.pin_wr = 8;   cfg.pin_rd = 9;   cfg.pin_rs = 7;
      cfg.pin_d0 = 39;  cfg.pin_d1 = 40;  cfg.pin_d2 = 41;  cfg.pin_d3 = 42;
      cfg.pin_d4 = 45;  cfg.pin_d5 = 46;  cfg.pin_d6 = 47;  cfg.pin_d7 = 48;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Dalle 170x320, décalée de 35 px
      auto cfg = _panel.config();
      cfg.pin_cs   = 6;
      cfg.pin_rst  = 5;
      cfg.panel_width  = 170;
      cfg.panel_height = 320;
      cfg.offset_x = 35;
      cfg.rgb_order = true;
      cfg.invert = true;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX tft;

// ---------- Encodeur ----------
volatile int compteur = 0;
volatile uint8_t etatEncodeur = 0;

const uint8_t tableEtats[7][4] = {
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10}, {0x3, 0x2, 0x0, 0x0},
  {0x3, 0x2, 0x1, 0x0}, {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
  {0x6, 0x5, 0x4, 0x0},
};

void IRAM_ATTR lireEncodeur() {
  uint8_t entrees = (digitalRead(ENCODER_PIN_B) << 1) | digitalRead(ENCODER_PIN_A);
  etatEncodeur = tableEtats[etatEncodeur & 0x0F][entrees];

  uint8_t dir = etatEncodeur & 0x30;
  if (dir == 0x10)      compteur -= ENCODER_SENS;
  else if (dir == 0x20) compteur += ENCODER_SENS;
}

// ---------- Affichage ----------
void afficherCompteur() {
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFont(&fonts::Font7);               // Gros chiffres 7 segments
  tft.drawString(String(compteur), tft.width() / 2, tft.height() / 2, 7);
}

void setup() {
  // ⚠️ Régulateur externe : sans ça, rien ne s'allume
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  Serial.begin(115200);

  tft.init();
  tft.setRotation(3);                        // Paysage 320x170
  tft.fillScreen(TFT_BLACK);

  // Rétroéclairage
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.drawString("ATS-OS", 10, 10);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), lireEncodeur, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), lireEncodeur, CHANGE);

  afficherCompteur();
}

void loop() {
  static int dernier = 0;

  if (compteur != dernier) {
    tft.fillRect(0, 50, tft.width(), 80, TFT_BLACK);  // Efface l'ancienne valeur
    afficherCompteur();
    dernier = compteur;
    Serial.printf("Compteur : %d\n", compteur);
  }

  if (digitalRead(ENCODER_PUSH_BUTTON) == LOW) {
    compteur = 0;
    delay(300);
  }

  delay(10);
}