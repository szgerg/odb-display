#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

static BLEAdvertisedDevice *targetDevice = nullptr;
static BLEClient *pClient = nullptr;
static BLERemoteService *pService = nullptr;
static BLERemoteCharacteristic *pRemoteChar = nullptr;
static bool connected = false;
static String response = "";

bool ble_init(const char *targetAddress = "74:fb:ac:f4:6e:1d")
{
  BLEDevice::init("ESP32-OBD");

  // Scan for BLE devices
  BLEScan *pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults *results = pScan->start(10);

  Serial.print("[BLE] Devices found: ");
  Serial.println(results->getCount());

  // Look for Vgate iCar Pro by MAC address
  for (int i = 0; i < results->getCount(); i++)
  {
    BLEAdvertisedDevice device = results->getDevice(i);
    String name = device.haveName() ? device.getName().c_str() : "(no name)";
    String addr = device.getAddress().toString().c_str();
    Serial.print("[BLE] Found: ");
    Serial.print(name);
    Serial.print(" | Addr: ");
    Serial.print(addr);
    if (device.haveServiceUUID())
    {
      for (int i = 0; i < device.getServiceUUIDCount(); i++)
      {
        Serial.print(" | Service: ");
        Serial.print(device.getServiceUUID(i).toString().c_str());
      }
    }
    Serial.println();

    // Match by MAC address
    if (addr.equalsIgnoreCase(targetAddress))
      targetDevice = new BLEAdvertisedDevice(device);
  }
  pScan->stop();

  if (targetDevice == nullptr)
  {
    Serial.println("[BLE] ERROR: OBD adapter not found. Make sure it's powered on (plugged into OBD port).");
    Serial.println("[BLE] Tip: Check device names above and adjust the name filter in code.");
    return false;
  }

  return true;
}

bool ble_connect()
{
  if (targetDevice == nullptr)
    return false;

  Serial.print("[BLE] Connecting to ");
  Serial.println(targetDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  if (!pClient->connect(targetDevice))
  {
    Serial.println("[BLE] ERROR: Connection failed!");
    return false;
  }
  Serial.println("[BLE] Connected to adapter!");

  return true;
}

bool ble_findService(const char *serviceUUIDStr)
{
  if (pClient == nullptr || !pClient->isConnected())
    return false;

  BLEUUID serviceUUID(serviceUUIDStr);
  pService = pClient->getService(serviceUUID);
  if (pService == nullptr)
  {
    Serial.println("[BLE] ERROR: Service not found. UUIDs may differ for your adapter.");
    Serial.println("[BLE] Tip: Use nRF Connect app to find correct UUIDs.");
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] Service found!");
  return true;
}

bool ble_findCharacteristics(void (*callback)(BLERemoteCharacteristic *, uint8_t *, size_t, bool), const char *rxCharUUIDStr, const char *txCharUUIDStr)
{
  if (pClient == nullptr || !pClient->isConnected())
    return false;

  if (pService == nullptr)
    return false;

  BLEUUID rxUUID(rxCharUUIDStr);
  BLEUUID txUUID(txCharUUIDStr);

  // Find the RX characteristic (for writing commands)
  pRemoteChar = pService->getCharacteristic(rxUUID);
  if (pRemoteChar == nullptr)
  {
    Serial.println("[BLE] ERROR: RX Characteristic not found.");
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] RX Characteristic found (write)!");

  // Find the TX characteristic (for receiving notifications)
  BLERemoteCharacteristic *pTxChar = pService->getCharacteristic(txUUID);
  if (pTxChar == nullptr)
  {
    Serial.println("[BLE] ERROR: TX Characteristic not found.");
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] TX Characteristic found (notify)!");

  // Subscribe to notifications on TX characteristic
  if (pTxChar->canNotify())
  {
    pTxChar->registerForNotify(callback);
    Serial.println("[BLE] Notifications enabled on TX.");
  }
  else
  {
    Serial.println("[BLE] WARNING: TX characteristic cannot notify!");
    return false;
  }

  return true;
}

void ble_sendCommand(const char *cmd)
{
  if (pRemoteChar == nullptr)
    return;
  String s = String(cmd) + "\r";
  pRemoteChar->writeValue((uint8_t *)s.c_str(), s.length());
  Serial.print("[OBD] Sent: ");
  Serial.println(cmd);
  delay(1000);
}
