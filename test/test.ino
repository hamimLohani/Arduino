#include <WiFi.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

// Most ESP32-S3 Dev boards have an addressable RGB LED (WS2812) on GPIO 48
#define RGB_LED_PIN 48

void setup() {
  Serial.begin(115200);
  delay(2000); // Allow time for USB Serial monitor to attach

  Serial.println("\n==========================================");
  Serial.println("       ESP32-S3 N16R8 HARDWARE TEST       ");
  Serial.println("==========================================");

  // 1. Chip & CPU Information
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  Serial.printf("Chip Model:        %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision:     %d\n", chip_info.revision);
  Serial.printf("CPU Cores:         %d\n", chip_info.cores);
  Serial.printf("CPU Frequency:     %u MHz\n", ESP.getCpuFreqMHz());

  // 2. Flash Memory Information
  uint32_t flash_size = 0;
  esp_flash_get_size(NULL, &flash_size);
  Serial.println("\n--- FLASH MEMORY ---");
  Serial.printf("Flash Size:        %u MB (%u bytes)\n", flash_size / (1024 * 1024), flash_size);
  Serial.printf("Flash Speed:       %u MHz\n", ESP.getFlashChipSpeed() / 1000000);

  // 3. PSRAM Information (N16R8 Specific Test)
  Serial.println("\n--- PSRAM TEST ---");
  if (psramFound()) {
    Serial.println("Status:            PSRAM Found & Initialized!");
    Serial.printf("Total PSRAM:       %u KB (%.2f MB)\n", ESP.getPsramSize() / 1024, ESP.getPsramSize() / (1024.0 * 1024.0));
    Serial.printf("Free PSRAM:        %u KB\n", ESP.getFreePsram() / 1024);

    // Read/Write verification test in PSRAM
    const size_t test_size = 1024 * 1024; // 1 MB test buffer
    Serial.printf("Allocating 1 MB test buffer in PSRAM... ");
    uint8_t *psram_buf = (uint8_t *)ps_malloc(test_size);

    if (psram_buf != NULL) {
      Serial.println("OK");
      Serial.print("Testing PSRAM read/write integrity... ");
      bool success = true;
      for (size_t i = 0; i < test_size; i++) {
        psram_buf[i] = (uint8_t)(i & 0xFF);
      }
      for (size_t i = 0; i < test_size; i++) {
        if (psram_buf[i] != (uint8_t)(i & 0xFF)) {
          success = false;
          break;
        }
      }
      free(psram_buf);

      if (success) {
        Serial.println("[PASSED]");
      } else {
        Serial.println("[FAILED] Data mismatch!");
      }
    } else {
      Serial.println("[FAILED] Memory allocation failed.");
    }
  } else {
    Serial.println("[WARNING] PSRAM NOT DETECTED!");
    Serial.println("Make sure 'PSRAM: OPI PSRAM' is selected in the Tools menu.");
  }

  // 4. Internal SRAM Information
  Serial.println("\n--- INTERNAL SRAM ---");
  Serial.printf("Total Heap:        %u bytes\n", ESP.getHeapSize());
  Serial.printf("Free Heap:         %u bytes\n", ESP.getFreeHeap());

  // 5. Wi-Fi RF Test
  Serial.println("\n--- WI-FI SCAN TEST ---");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Scanning for networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("Found %d network(s):\n", n);
    for (int i = 0; i < n && i < 5; ++i) { // Show up to 5 networks
      Serial.printf("  %d: %s (Signal: %d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }

  Serial.println("\n==========================================");
  Serial.println("  Hardware test complete. Blinking LED... ");
  Serial.println("==========================================\n");
}

void loop() {
  // Built-in ESP32 RGB LED control (RGB: 0-255)
  // Red
  rgbLedWrite(RGB_LED_PIN, 40, 0, 0);
  delay(500);

  // Green
  rgbLedWrite(RGB_LED_PIN, 0, 40, 0);
  delay(500);

  // Blue
  rgbLedWrite(RGB_LED_PIN, 0, 0, 40);
  delay(500);

  // Off
  rgbLedWrite(RGB_LED_PIN, 0, 0, 0);
  delay(500);
}
