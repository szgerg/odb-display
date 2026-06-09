#pragma once
#include <esp_task_wdt.h>

static bool wdt_subscribed = false;

void wdt_init(uint32_t timeout_s, bool panic_on_timeout) {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = timeout_s * 1000,
    .idle_core_mask = 0,
    .trigger_panic = panic_on_timeout
  };
  esp_task_wdt_reconfigure(&wdt_config);
  if (!wdt_subscribed) {
    esp_task_wdt_add(NULL);
    wdt_subscribed = true;
  }

  Serial.print("[WDT] Initialized, timeout: ");
  Serial.print(timeout_s);
  Serial.print("s, panic: ");
  Serial.println(panic_on_timeout ? "yes" : "no");
}

void wdt_reset() {
  esp_task_wdt_reset();
  Serial.println("[WDT] Watchdog reset.");
}
