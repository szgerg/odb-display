#include "watchdog.h"
#include "serial.h"
#include "ble.h"
#include "elm327.h"
#include "display.h"

const char *serviceUUIDstr = "000018f0-0000-1000-8000-00805f9b34fb";
const char *rxCharUUIDstr = "00002af0-0000-1000-8000-00805f9b34fb";
const char *txCharUUIDstr = "00002af1-0000-1000-8000-00805f9b34fb";
const char *address = "41:42:86:9a:5a:d3";

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
    }
    else if (c != '\r' && c != '\n')
    {
      response += c;
    }
  }

  waiting = false;
}

void setup()
{
  serial_init();

  tft_init();

  Serial.println("[SYSTEM] Initializing...");
  tft_write_center("Initializing...");

  wdt_init(30, true);

  if (!ble_init(address))
    return;

  wdt_reset();

  Serial.println("[SYSTEM] Connecting...");
  tft_write_center("Connecting...");

  if (!ble_connect())
    return;

  wdt_reset();

  if (!ble_findService(serviceUUIDstr))
    return;

  wdt_reset();

  if (!ble_findCharacteristics(notifyCallback, rxCharUUIDstr, txCharUUIDstr))
    return;

  wdt_reset();

  tft_write_center("Success.");

  elm_init();

  wdt_reset();

  wdt_init(10, true);
  tft_write_center("Fetching data...");
}

void loop()
{
  // delay(1000);
  // Serial.println("\n--- Polling ---");

  // sendCommand("010C"); // Engine RPM
  // waitForResponse();
  // sendCommand("010D"); // Vehicle speed
  // waitForResponse();
  // sendCommand("0105"); // Coolant temperature
  // waitForResponse();
  // sendCommand("012F"); // Fuel level
  // waitForResponse();

  // Serial.printf("[DATA] RPM: %d, Speed: %d km/h, Coolant: %d°C, Fuel: %d%%\n", rpm, speed, coolantTemp, fuelLevel);
}
