#include <esp_task_wdt.h>

void wdt_init(uint32_t timeout_s, bool panic_on_timeout) {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = timeout_s * 1000,
    .idle_core_mask = 0,
    .trigger_panic = panic_on_timeout
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  Serial.print("[WDT] Initialized, timeout: ");
  Serial.print(timeout_s);
  Serial.print("s, panic: ");
  Serial.println(panic_on_timeout ? "yes" : "no");
}

void serial_init() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[WDT] Serial initialized.");
}

void setup() {
    
  serial_init();

  wdt_init(3, true);

  Serial.println("[WDT] Watchdog initialized. Increasing delay between resets...");

  int delayMs = 500;
  int step = 500;

  while (true) {
    Serial.print("[WDT] Resetting watchdog, next delay: ");
    Serial.print(delayMs);
    Serial.println(" ms");

    esp_task_wdt_reset();
    delay(delayMs);

    delayMs += step;
  }
}

void loop() {
  // Never reached - reset happens in setup loop
}
