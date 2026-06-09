void elm_init()
{
    Serial.println("[ELM327] Initializing ELM327...");
    delay(500);
    ble_sendCommand("ATZ"); // Reset ELM327
    delay(2000);
    ble_sendCommand("ATE0");  // Echo off
    ble_sendCommand("ATL0");  // Linefeeds off
    ble_sendCommand("ATS0");  // Spaces off
    ble_sendCommand("ATH0");  // Headers off
    ble_sendCommand("ATSP0"); // Auto-detect protocol
}