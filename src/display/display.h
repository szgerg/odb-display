#include <LovyanGFX.hpp>
#include "icons.h"
#include "drop.h"

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

LGFX tft;

void tft_init()
{
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
}

void tft_clear()
{
    tft.fillScreen(TFT_BLACK);
}

void tft_write_center(const char *text, uint16_t color = TFT_WHITE)
{
    tft_clear();
    int w = tft.textWidth(text);
    int h = tft.fontHeight();
    tft.setCursor((tft.width() - w) / 2, 110);
    tft.setTextColor(color);
    tft.print(text);
}

void tft_write_data(int rpm, int speed, int coolant, int fuel, bool clear = false)
{
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    int x = 60;
    int tx = x + 30;

    int rel = 6;
    int yofff = 50;
    int deltaY = 40;

    if (clear)
        tft_clear();

    tft.fillRect(x, yofff + 0 * deltaY, 200, deltaY, TFT_BLACK);

    tft.drawBitmap(x, yofff + 0 * deltaY, bitmap_rpm, 24, 24, TFT_BLACK, TFT_BROWN);
    tft.setCursor(tx, yofff + rel + 0 * deltaY);
    tft.print(rpm);
    tft.println("rpm");

    tft.fillRect(x, yofff + 1 * deltaY, 200, deltaY, TFT_BLACK);
    tft.drawBitmap(x, yofff + 1 * deltaY, bitmap_speed, 24, 24, TFT_BLACK, TFT_DARKSLATEBLUE);
    tft.setCursor(tx, yofff + rel + 1 * deltaY);
    tft.print(speed);
    tft.println("km/h");

    tft.fillRect(x, yofff + 2 * deltaY, 200, deltaY, TFT_BLACK);
    tft.drawBitmap(x, yofff + 2 * deltaY, bitmap_coolant, 24, 24, TFT_BLACK, TFT_DARKOLIVEGREEN);
    tft.setCursor(tx, yofff + rel + 2 * deltaY);
    tft.print(coolant);
    tft.println("C");

    tft.fillRect(x, yofff + 3 * deltaY, 200, deltaY, TFT_BLACK);
    tft.drawBitmap(x, yofff + 3 * deltaY, bitmap_fuel, 24, 24, TFT_BLACK, TFT_GREEN);
    tft.setCursor(tx, yofff + rel + 3 * deltaY);
    tft.print(fuel);
    tft.println("%");
}

void tft_write_coolant_high(int temp)
{
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(75, 45, drop, 90, 90, TFT_RED);

    tft.setTextColor(TFT_RED, TFT_BLACK);
    int w = tft.textWidth("Temp HIGH!");
    tft.drawString("Temp HIGH!", (tft.width() - w) / 2, 170);
}