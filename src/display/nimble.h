#include <NimBLEDevice.h>

#define PASSKEY 1234

static NimBLEAdvertisedDevice *targetDevice = nullptr;
static NimBLEClient *pClient = nullptr;
static NimBLERemoteService *pService = nullptr;
static NimBLERemoteCharacteristic *pRemoteChar = nullptr;
static bool connected = false;
static String response = "";

bool ble_init(const char *targetAddress)
{
  NimBLEDevice::init("ESP32-OBD");

  Serial.println("[BLE] Starting scan...");

  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->start(10);
  while (pScan->isScanning())
  {
    delay(100);
  }
  NimBLEScanResults results = pScan->getResults();

  Serial.print("[BLE] Devices found: ");
  Serial.println(results.getCount());

  for (int i = 0; i < results.getCount(); i++)
  {
    const NimBLEAdvertisedDevice *device = results.getDevice(i);
    String name = device->haveName() ? device->getName().c_str() : "(no name)";
    String addr = device->getAddress().toString().c_str();
    Serial.print("[BLE] Found: ");
    Serial.print(name);
    Serial.print(" | Addr: ");
    Serial.print(addr);
    Serial.print(" | AddrType: ");
    Serial.println(device->getAddress().getType());

    if (addr.equalsIgnoreCase(targetAddress))
    {
      targetDevice = new NimBLEAdvertisedDevice(*device);
      break;
    }
  }
  pScan->stop();
  pScan->clearResults();
  delay(500);

  if (targetDevice == nullptr)
  {
    Serial.println("[BLE] ERROR: OBD adapter not found. Make sure it's powered on (plugged into OBD port).");
    return false;
  }

  return true;
}

class MyClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient)
  {
    Serial.println("[BLE] >>> onConnect callback fired");
  }
  void onDisconnect(NimBLEClient *pClient, int reason)
  {
    Serial.print("[BLE] >>> onDisconnect, reason: 0x");
    Serial.println(reason, HEX);
    connected = false;
  }
};

bool ble_connect()
{
  if (targetDevice == nullptr)
    return false;

  Serial.print("[BLE] Connecting to: ");
  Serial.println(targetDevice->getAddress().toString().c_str());

  for (int attempt = 1; attempt <= 3; attempt++)
  {
    Serial.print("[BLE] Attempt ");
    Serial.print(attempt);
    Serial.println("/3");

    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());
    pClient->setConnectionParams(24, 48, 0, 400); // relaxed: 30-60ms interval, 4s timeout
    pClient->setConnectTimeout(15);

    if (pClient->connect(targetDevice))
    {
      Serial.print("[BLE] Connected! MTU: ");
      Serial.println(pClient->getMTU());
      connected = true;
      return true;
    }

    Serial.println("[BLE] connect() failed.");

    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;

    delay(2000 * attempt);
  }

  Serial.println("[BLE] All connection attempts failed.");
  return false;
}

bool ble_findService(const char *serviceUUIDStr)
{
  Serial.println("[BLE] Find Service...");

  if (pClient == nullptr || !pClient->isConnected())
    return false;

  NimBLEUUID serviceUUID(serviceUUIDStr);
  pService = pClient->getService(serviceUUID);
  if (pService == nullptr)
  {
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] Service found!");
  return true;
}

bool ble_findCharacteristics(void (*callback)(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool))
{
  Serial.println("[BLE] Find Characteristics...");

  if (pClient == nullptr || !pClient->isConnected())
    return false;

  if (pService == nullptr)
    return false;

  NimBLERemoteCharacteristic *pTxChar = nullptr;

  auto chars = pService->getCharacteristics(true);
  if (chars.empty())
  {
    Serial.println("[BLE] ERROR: No characteristics found.");
    pClient->disconnect();
    return false;
  }

  for (auto *c : chars)
  {
    Serial.print("[BLE]   Characteristic: ");
    Serial.print(c->getUUID().toString().c_str());
    Serial.print(" | canWrite: ");
    Serial.print(c->canWrite() || c->canWriteNoResponse());
    Serial.print(" | canNotify: ");
    Serial.println(c->canNotify());

    if ((c->canWrite() || c->canWriteNoResponse()) && pRemoteChar == nullptr)
    {
      pRemoteChar = c;
      Serial.println("[BLE] RX Characteristic found (write)!");
    }

    if (c->canNotify() && pTxChar == nullptr)
    {
      pTxChar = c;
      Serial.println("[BLE] TX Characteristic found (notify)!");
    }
  }

  if (pRemoteChar == nullptr)
  {
    Serial.println("[BLE] ERROR: RX Characteristic not found.");
    pClient->disconnect();
    return false;
  }

  if (pTxChar == nullptr)
  {
    Serial.println("[BLE] ERROR: TX Characteristic not found.");
    pClient->disconnect();
    return false;
  }

  pTxChar->subscribe(true, callback);
  Serial.println("[BLE] Notifications enabled on TX.");

  return true;
}

void ble_sendCommand(const char *cmd)
{
  if (pRemoteChar == nullptr)
    return;

  String s = String(cmd) + "\r";
  pRemoteChar->writeValue((uint8_t *)s.c_str(), s.length(), false);
  Serial.print("[OBD] Sent: ");
  Serial.println(cmd);
}
