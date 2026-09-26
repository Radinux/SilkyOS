#include "Config.h"
#include "Display.h"
#include "Storage.h"

// ---------- Configuration matérielle de l'écran ----------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;

public:
  LGFX() {
    { // Bus parallèle 8 bits
      auto cfg = _bus.config();
      cfg.freq_write = 20000000;
      cfg.pin_wr = 8;   cfg.pin_rd = 9;   cfg.pin_rs = 7;
      cfg.pin_d0 = 39;  cfg.pin_d1 = 40;  cfg.pin_d2 = 41;  cfg.pin_d3 = 42;
      cfg.pin_d4 = 45;  cfg.pin_d5 = 46;  cfg.pin_d6 = 47;  cfg.pin_d7 = 48;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Dalle 170x320
      auto cfg = _panel.config();
      cfg.pin_cs = 6;   cfg.pin_rst = 5;
      cfg.panel_width  = 170;
      cfg.panel_height = 320;
      cfg.offset_x   = 35;
      cfg.rgb_order  = false;
      cfg.invert     = true;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

static LGFX tft;

const Theme TH = {
  .fond    = TFT_BLACK,
  .entete  = 0x2104,
  .texte   = TFT_WHITE,
  .accent1 = TFT_CYAN,
  .accent2 = TFT_GREEN,
  .accent3 = TFT_ORANGE,
};

// ---------- API ----------
void displayInit() {
  // Régulateur externe : indispensable avant tout
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  tft.init();
  tft.setRotation((ROTATION_BASE + reglages.rotation) % 4);
  tft.fillScreen(TH.fond);

  // Rétroéclairage en PWM (canal 0, 5 kHz, 8 bits)
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  displaySetBrightness(reglages.luminosite);
}

void displaySetBrightness(uint8_t niveau) {
  // Plancher à 10 : évite un écran noir irrécupérable
  ledcWrite(0, max((uint8_t)10, niveau));
}

void displaySetRotation(uint8_t rotation) {
  tft.setRotation((ROTATION_BASE + rotation) % 4);   // Rotation matérielle (MADCTL)
}

int displayWidth()  { return tft.width();  }
int displayHeight() { return tft.height(); }

// Envoie une zone rendue par LVGL vers l'écran
void displayFlush(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *px) {
  tft.pushImage(x, y, w, h, (lgfx::rgb565_t *)px);
}

void displayEteindre() {
  ledcWrite(0, 0);          // Vraiment 0, sans le plancher de sécurité de displaySetBrightness()
}