#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

// Nordic UART Service (NUS) UUIDs
static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID rxCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");  // Write to device
static BLEUUID txCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");  // Notify from device

static BLEClient* pClient = nullptr;
static BLERemoteCharacteristic* pRemoteChar = nullptr;
static bool connected = false;
static String response = "";

// Notification callback - receives data from ELM327
static void notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("[OBD] Notify received, length: ");
  Serial.println(length);
  for (size_t i = 0; i < length; i++) {
    char c = (char)pData[i];
    if (c == '>') {
      // '>' is the ELM327 prompt, response is complete
      Serial.print("[OBD] Response: ");
      Serial.println(response);
      response = "";
    } else if (c != '\r' && c != '\n') {
      response += c;
    }
  }
}

// Send an AT/OBD command to the adapter
void sendCommand(const char* cmd) {
  if (pRemoteChar == nullptr) return;
  String s = String(cmd) + "\r";
  pRemoteChar->writeValue((uint8_t*)s.c_str(), s.length());
  Serial.print("[OBD] Sent: ");
  Serial.println(cmd);
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[BLE] Starting BLE scan...");

  BLEDevice::init("ESP32-OBD");

  // Scan for BLE devices
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults* results = pScan->start(10);

  Serial.print("[BLE] Devices found: ");
  Serial.println(results->getCount());

  //Look for Vgate iCar Pro by MAC address
  BLEAdvertisedDevice* targetDevice = nullptr;
  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    String name = device.haveName() ? device.getName().c_str() : "(no name)";
    String addr = device.getAddress().toString().c_str();
    Serial.print("[BLE] Found: ");
    Serial.print(name);
    Serial.print(" | Addr: ");
    Serial.print(addr);
    if (device.haveServiceUUID()) {
      for (int i = 0; i < device.getServiceUUIDCount(); i++) {
        Serial.print(" | Service: ");
        Serial.print(device.getServiceUUID(i).toString().c_str());
      }
    }
    Serial.println();

    // Match by MAC address
    if (addr.equalsIgnoreCase("74:fb:ac:f4:6e:1d")) {
      targetDevice = new BLEAdvertisedDevice(device);
      Serial.println("[BLE] >>> Target OBD adapter found by address!");
    }
  }
  pScan->stop();

  if (targetDevice == nullptr) {
    Serial.println("[BLE] ERROR: OBD adapter not found. Make sure it's powered on (plugged into OBD port).");
    Serial.println("[BLE] Tip: Check device names above and adjust the name filter in code.");
    return;
  }

  // Connect to the adapter
  Serial.println("[BLE] Connecting...");
  pClient = BLEDevice::createClient();
  pClient->connect(targetDevice);

  if (!pClient->isConnected()) {
    Serial.println("[BLE] ERROR: Connection failed!");
    return;
  }
  Serial.println("[BLE] Connected!");

  // Find the service
  BLERemoteService* pService = pClient->getService(serviceUUID);
  if (pService == nullptr) {
    Serial.println("[BLE] ERROR: Service not found. UUIDs may differ for your adapter.");
    Serial.println("[BLE] Tip: Use nRF Connect app to find correct UUIDs.");
    pClient->disconnect();
    return;
  }
  Serial.println("[BLE] Service found!");

  // List all characteristics
  std::map<std::string, BLERemoteCharacteristic*>* chars = pService->getCharacteristics();
  for (auto& entry : *chars) {
    Serial.print("[BLE] Characteristic: ");
    Serial.print(entry.second->getUUID().toString().c_str());
    Serial.print(" | canRead: ");
    Serial.print(entry.second->canRead());
    Serial.print(" | canWrite: ");
    Serial.print(entry.second->canWrite());
    Serial.print(" | canNotify: ");
    Serial.println(entry.second->canNotify());
  }

  // Find the RX characteristic (for writing commands)
  pRemoteChar = pService->getCharacteristic(rxCharUUID);
  if (pRemoteChar == nullptr) {
    Serial.println("[BLE] ERROR: RX Characteristic not found.");
    pClient->disconnect();
    return;
  }
  Serial.println("[BLE] RX Characteristic found (write)!");

  // Find the TX characteristic (for receiving notifications)
  BLERemoteCharacteristic* pTxChar = pService->getCharacteristic(txCharUUID);
  if (pTxChar == nullptr) {
    Serial.println("[BLE] ERROR: TX Characteristic not found.");
    pClient->disconnect();
    return;
  }
  Serial.println("[BLE] TX Characteristic found (notify)!");

  // Subscribe to notifications on TX characteristic
  if (pTxChar->canNotify()) {
    pTxChar->registerForNotify(notifyCallback);
    Serial.println("[BLE] Notifications enabled on TX.");
  } else {
    Serial.println("[BLE] WARNING: TX characteristic cannot notify!");
  }

  connected = true;
  Serial.println("");
  Serial.println("=============================");
  Serial.println("  Success");
  Serial.println("=============================");
  Serial.println("");

  // Initialize ELM327
  delay(500);
  sendCommand("ATZ");       // Reset ELM327
  delay(2000);
  sendCommand("ATE0");      // Echo off
  sendCommand("ATL0");      // Linefeeds off
  sendCommand("ATS0");      // Spaces off
  sendCommand("ATH0");      // Headers off
  sendCommand("ATSP0");     // Auto-detect protocol

  // Query some OBD data
  Serial.println("\n[OBD] Querying vehicle data...");
  sendCommand("0100");      // Supported PIDs [01-20]
  sendCommand("010C");      // Engine RPM
  sendCommand("010D");      // Vehicle speed
  sendCommand("0105");      // Coolant temperature
  sendCommand("012F");      // Fuel level
}

void loop() {
  if (!connected) return;

  // Periodically query RPM and speed
  delay(5000);
  Serial.println("\n--- Polling ---");
  sendCommand("010C");      // RPM
  sendCommand("010D");      // Speed
}
