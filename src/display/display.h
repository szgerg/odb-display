#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI _bus;

public:
    LGFX()
    {
        auto cfg = _bus.config();

        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;

        // TRY THIS ALTERNATE PIN SET (VERY IMPORTANT)
        cfg.pin_sclk = 6;
        cfg.pin_mosi = 7;
        cfg.pin_miso = -1;
        cfg.pin_dc = 2;

        cfg.freq_write = 20000000;

        _bus.config(cfg);
        _panel.setBus(&_bus);

        auto pcfg = _panel.config();

        pcfg.pin_cs = 10;
        pcfg.pin_rst = 3;
        pcfg.pin_busy = -1;

        pcfg.panel_width = 240;
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

void tft_init()
{
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
}

void tft_write_center(const char *text)
{
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((tft.width() - w) / 2, 110);
    tft.setTextColor(TFT_YELLOW);
    tft.print(text);
}