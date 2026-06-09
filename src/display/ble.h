#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define PASSKEY 1234

// Security callback required for Passkey authentication
class MySecurity : public BLESecurityCallbacks
{
  uint32_t onPassKeyRequest() { return PASSKEY; }
  void onPassKeyNotify(uint32_t pass_key) {}
  bool onConfirmPIN(uint32_t pass_key) { return true; }
  bool onSecurityRequest() { return true; }
  void onAuthenticationComplete(ble_gap_conn_desc *desc)
  {
    Serial.println(desc->sec_state.encrypted ? "Auth Success!" : "Auth Failed");
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
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity security;
  security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  security.setCapability(ESP_IO_CAP_NONE);

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
  pClient->secureConnection();

  bool success = pClient->connect(targetDevice);
  if (success && pClient->isConnected())
  {
    Serial.print("[BLE] Connected! MTU: ");
    Serial.println(pClient->getMTU());

    delay(3000);

    if (pClient->isConnected())
    {
      Serial.println("[BLE] Connected to adapter!");
      return true;
    }
    Serial.println("[BLE] Connection lost after pairing!");
  }

  return false;
}

bool ble_findService(const char *serviceUUIDStr)
{
  Serial.println("[BLE] Find Service...");

  if (pClient == nullptr || !pClient->isConnected())
    return false;

  // Allow time for connection to stabilize after pairing
  delay(2000);

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

bool ble_findCharacteristics(void (*callback)(BLERemoteCharacteristic *, uint8_t *, size_t, bool), const char *rxCharUUIDStr, const char *txCharUUIDStr)
{
  Serial.println("[BLE] Find Characteristics...");

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
