#include "watchdog.h"
#include "serial.h"
#include "ble.h"
#include "elm327.h"

const char *serviceUUIDstr = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char *rxCharUUIDstr = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const char *txCharUUIDstr = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

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
      response = "";
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

  wdt_init(30, true);

  if (!ble_init("7a:ed:18:1f:dc:b9"))
    return;

  wdt_reset();
  wdt_init(10, true);

  if (!ble_connect())
    return;

  wdt_reset();

  if (!ble_findService(serviceUUIDstr))
    return;

  wdt_reset();

  if (!ble_findCharacteristics(notifyCallback, rxCharUUIDstr, txCharUUIDstr))
    return;

  wdt_reset();

  connected = true;
  Serial.println("");
  Serial.println("=============================");
  Serial.println("  Success");
  Serial.println("=============================");
  Serial.println("");

  // Initialize ELM327
  elm_init();

  // Query some OBD data
  Serial.println("\n[OBD] Querying vehicle data...");
  ble_sendCommand("0100"); // Supported PIDs [01-20]
  ble_sendCommand("010C"); // Engine RPM
  ble_sendCommand("010D"); // Vehicle speed
  ble_sendCommand("0105"); // Coolant temperature
  ble_sendCommand("012F"); // Fuel level

  wdt_reset();
}

void loop()
{
  delay(1000);
  Serial.println("\n--- Polling ---");
  ble_sendCommand("010C"); // RPM
  ble_sendCommand("010D"); // Speed

  wdt_reset();
}
