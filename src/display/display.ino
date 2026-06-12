#include "watchdog.h"
#include "serial.h"
#include "nimble.h"
#include "elm327.h"
#include "tft.h"

#include "hourglass.h"
#include "bluetooth.h"
#include "drop.h"
#include "search.h"
#include "data-transfer.h"

const char *serviceUUIDstr = "000018f0-0000-1000-8000-00805f9b34fb";
// const char *address = "41:42:86:9a:5a:d3";
const char *address = "6c:29:35:1a:7b:f7";

volatile bool waiting = false;

int rpm = 0;
int speed = 0;
int coolantTemp = 0;
int fuelLevel = 0;

void waitForResponse()
{
  while (waiting)
  {
    delay(100);
  }

  wdt_reset();
}

void sendCommand(const char *cmd)
{
  waiting = true;
  ble_sendCommand(cmd);
}

static void notifyCallback(BLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify)
{
  Serial.print("[OBD] Notify received, length: ");
  Serial.println(length);
  for (size_t i = 0; i < length; i++)
  {
    char c = (char)pData[i];
    if (c == '>')
    {
      // '>' is the ELM327 prompt, response is complete
      Serial.print("[OBD] Response: ");
      Serial.println(response);

      if (response.startsWith("410C") && response.length() >= 8)
      {
        int a = strtol(response.substring(4, 6).c_str(), NULL, 16);
        int b = strtol(response.substring(6, 8).c_str(), NULL, 16);
        rpm = (256 * a + b) / 4;
      }
      else if (response.startsWith("410D") && response.length() >= 6)
      {
        speed = strtol(response.substring(4, 6).c_str(), NULL, 16);
      }
      else if (response.startsWith("4105") && response.length() >= 6)
      {
        coolantTemp = strtol(response.substring(4, 6).c_str(), NULL, 16) - 40;
      }
      else if (response.startsWith("412F") && response.length() >= 6)
      {
        fuelLevel = (100 * strtol(response.substring(4, 6).c_str(), NULL, 16)) / 255;
      }

      response = "";
      waiting = false;
    }
    else if (c != '\r' && c != '\n')
    {
      response += c;
    }
  }
}

void setup()
{
  serial_init();

  tft_init();

  Serial.println("[SYSTEM] Initializing...");
  tft_write_icon_text(bitmap_hourglass, "Initializing...", TFT_YELLOW);
  delay(8000);

  wdt_init(15, true);

  Serial.println("[SYSTEM] Finding device...");
  tft_write_icon_text(bitmap_search, "Finding device...", TFT_MAGENTA);

  if (!ble_init(address))
    return;

  wdt_reset();
  
  Serial.println("[SYSTEM] Connecting...");
  tft_write_icon_text(bitmap_bluetooth, "Connecting...", TFT_BLUE);

  if (!ble_connect())
    return;

  wdt_reset();

  if (!ble_findService(serviceUUIDstr))
    return;

  wdt_reset();

  if (!ble_findCharacteristics(notifyCallback))
    return;

  wdt_reset();

  Serial.println("[SYSTEM] Fetching data...");
  tft_write_icon_text(bitmap_data_transfer, "Fetching data...", TFT_CYAN);

  elm_init();

  wdt_reset();
}

bool clear = true;

void loop()
{
  delay(1000);
  Serial.println("\n--- Polling ---");

  sendCommand("010C"); // Engine RPM
  waitForResponse();
  sendCommand("010D"); // Vehicle speed
  waitForResponse();
  sendCommand("0105"); // Coolant temperature
  waitForResponse();
  sendCommand("012F"); // Fuel level
  waitForResponse();

  Serial.printf("[DATA] RPM: %d, Speed: %d km/h, Coolant: %d°C, Fuel: %d%%\n", rpm, speed, coolantTemp, fuelLevel);

  if (coolantTemp >= 98)
  {
    tft_write_icon_text(bitmap_drop, "Temp HIGH!", TFT_RED);
    clear = true;
  }
  else
  {
    tft_write_data(rpm, speed, coolantTemp, fuelLevel, clear);
    clear = false;
  }

  wdt_reset();
}
