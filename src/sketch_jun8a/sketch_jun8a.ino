#include <LovyanGFX.hpp>
#include <drop.h>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX() {
    auto cfg = _bus.config();

    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;

    // TRY THIS ALTERNATE PIN SET (VERY IMPORTANT)
    cfg.pin_sclk = 6;
    cfg.pin_mosi = 7;
    cfg.pin_miso = -1;
    cfg.pin_dc   = 2;

    cfg.freq_write = 20000000;

    _bus.config(cfg);
    _panel.setBus(&_bus);

    auto pcfg = _panel.config();

    pcfg.pin_cs  = 10;
    pcfg.pin_rst = 3;
    pcfg.pin_busy = -1;

    pcfg.panel_width  = 240;
    pcfg.panel_height = 240;

    pcfg.offset_rotation = 0;

    pcfg.offset_x = 0;
    pcfg.offset_y = 0;

    pcfg.rgb_order = false;
    pcfg.invert = true;

    _panel.config(pcfg);
    setPanel(&_panel);
  }
};

LGFX tft;

const unsigned char* epd_bitmap_allArray[1] = {
	drop
};


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("START");

  tft.init();
  tft.setRotation(0);
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);

  // Draw the 90x90 drop icon centered on 240x240 screen
  // 1-bit bitmap: 1=icon shape, 0=background (transparent)
  tft.drawBitmap(75, 45, drop, 90, 90, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Temp OK.", 120, 170);

  delay(3000);

  // Clear and redraw in red
  tft.fillScreen(TFT_BLACK);
  tft.drawBitmap(75, 45, drop, 90, 90, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Temp HIGH!", 120, 170);

  tft.endWrite();
  Serial.println("DONE");
}

void loop() {}