#include "Config.h"
#include "Display.h"

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
      cfg.offset_x  = 35;
      cfg.rgb_order = false;
      cfg.invert    = true;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

// ---------- Définitions des globales ----------
static LGFX  tft;
LGFX_Sprite  spr(&tft);

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
  tft.setRotation(2);              // Portrait 170x320
  tft.fillScreen(TH.fond);

  // Buffer plein écran en PSRAM (170*320*2 = 108 ko)
  spr.setPsram(true);
  spr.setColorDepth(16);
  spr.createSprite(tft.width(), tft.height());

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
}

void displayPush() {
  spr.pushSprite(0, 0);
}