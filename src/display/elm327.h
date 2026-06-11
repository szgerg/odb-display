void elm_init()
{
    Serial.println("[ELM327] Initializing ELM327...");
    ble_sendCommand("ATZ");  // Reset ELM327
    delay(2000);             // ATZ needs extra time for reset
    ble_sendCommand("ATE0"); // Echo off
    delay(500);
    ble_sendCommand("ATL0"); // Linefeeds off
    delay(500);
    ble_sendCommand("ATS0"); // Spaces off
    delay(500);
    ble_sendCommand("ATH0"); // Headers off
    delay(500);
    ble_sendCommand("ATSP0"); // Auto-detect protocol
    delay(500);
}