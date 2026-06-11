#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define PASSKEY 1234

// Security callback required for Passkey authentication
class MySecurity : public BLESecurityCallbacks
{
  uint32_t onPassKeyRequest()
  {
    Serial.println("[BLE] Passkey requested, providing: " + String(PASSKEY));
    return PASSKEY;
  }
  void onPassKeyNotify(uint32_t pass_key)
  {
    Serial.print("[BLE] Passkey notify: ");
    Serial.println(pass_key);
  }
  bool onConfirmPIN(uint32_t pass_key)
  {
    Serial.print("[BLE] Confirm PIN: ");
    Serial.println(pass_key);
    return true;
  }
  bool onSecurityRequest()
  {
    Serial.println("[BLE] Security request received");
    return true;
  }
  void onAuthenticationComplete(ble_gap_conn_desc *desc)
  {
    Serial.println(desc->sec_state.encrypted ? "[BLE] Auth Success!" : "[BLE] Auth Failed");
    Serial.print("[BLE] Bonded: ");
    Serial.println(desc->sec_state.bonded);
  }
};

static BLEAdvertisedDevice *targetDevice = nullptr;
static BLEClient *pClient = nullptr;
static BLERemoteService *pService = nullptr;
static BLERemoteCharacteristic *pRemoteChar = nullptr;
static bool connected = false;
static String response = "";

bool ble_init(const char *targetAddress)
{
  BLEDevice::init("ESP32-OBD");

  // Clear bonding data via NimBLE's ble_store_clear to avoid stale pairing
  ble_store_clear();
  Serial.println("[BLE] Cleared bond store.");

  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity security;
  // Bond + MITM via passkey; no SC requirement for max compatibility with BT 5.3 adapters
  security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  // KeyboardOnly: we provide the passkey via onPassKeyRequest callback
  security.setCapability(ESP_IO_CAP_IN);

  // Scan for BLE devices
  BLEScan *pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults *results = pScan->start(10);

  Serial.print("[BLE] Devices found: ");
  Serial.println(results->getCount());

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

  Serial.println("[BLE] Trying to connect to adapter...");
  Serial.print("[BLE] Address: ");
  Serial.println(targetDevice->getAddress().toString().c_str());
  Serial.print("[BLE] Address type: ");
  Serial.println(targetDevice->getAddress().getType());

  pClient = BLEDevice::createClient();

  // Vgate iCar Pro 2S uses a Random address (0x41:xx = RPA pattern)
  // Try BLE_ADDR_RANDOM first, then public as fallback
  uint8_t addrTypes[] = {
      BLE_ADDR_PUBLIC,
      BLE_ADDR_RANDOM,
      2,3,4,5,6,7,8,9, 10,11,12,13,14,15};

  bool success = false;
  for (int attempt = 0; attempt < 16 && !success; attempt++)
  {
    uint8_t addrType = addrTypes[attempt];

    Serial.print("[BLE] Connection attempt #");
    Serial.print(attempt + 1);
    Serial.print(" with  address type: ");
    Serial.println(addrType);

    BLEAddress addr(targetDevice->getAddress().toString().c_str(), addrType);
    success = pClient->connect(addr);

    if (!success || !pClient->isConnected())
    {
      Serial.println("[BLE] Attempt failed, retrying...");
      success = false;
      delay(1000);
    }
  }

  if (success && pClient->isConnected())
  {
    Serial.print("[BLE] Connected! MTU: ");
    Serial.println(pClient->getMTU());

    // Wait for pairing/bonding to complete
    delay(2000);

    if (pClient->isConnected())
    {
      Serial.println("[BLE] Connected to adapter!");
      return true;
    }
    Serial.println("[BLE] Connection lost after pairing!");
  }
  else
  {
    Serial.println("[BLE] Connection failed after all attempts!");
  }

  return false;
}

bool ble_findService(const char *serviceUUIDStr)
{
  Serial.println("[BLE] Find Service...");

  if (pClient == nullptr || !pClient->isConnected())
    return false;

  // Allow time for connection to stabilize after pairing
  // delay(2000);

  Serial.println("[BLE] Discovering services...");
  BLEUUID serviceUUID(serviceUUIDStr);
  pService = pClient->getService(serviceUUID);
  if (pService == nullptr)
  {
    // Try listing all services for debugging
    Serial.println("[BLE] ERROR: Service not found. Listing discovered services:");
    auto *services = pClient->getServices();
    if (services)
    {
      for (auto &pair : *services)
      {
        Serial.print("[BLE]   Service: ");
        Serial.println(pair.second->getUUID().toString().c_str());
      }
    }
    pClient->disconnect();
    return false;
  }
  Serial.println("[BLE] Service found!");
  return true;
}

bool ble_findCharacteristics(void (*callback)(BLERemoteCharacteristic *, uint8_t *, size_t, bool))
{
  Serial.println("[BLE] Find Characteristics...");

  if (pClient == nullptr || !pClient->isConnected())
    return false;

  if (pService == nullptr)
    return false;

  BLERemoteCharacteristic *pTxChar = nullptr;

  auto *chars = pService->getCharacteristics();
  if (chars == nullptr)
  {
    Serial.println("[BLE] ERROR: No characteristics found.");
    pClient->disconnect();
    return false;
  }

  for (auto &pair : *chars)
  {
    BLERemoteCharacteristic *c = pair.second;
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

  // Subscribe to notifications on TX characteristic
  pTxChar->registerForNotify(callback);
  Serial.println("[BLE] Notifications enabled on TX.");

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
}
