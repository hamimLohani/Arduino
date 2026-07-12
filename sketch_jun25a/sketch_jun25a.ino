#include <Arduino.h>
#include <WiFi.h>
#include <esp_chip_info.h>
#include <esp_mac.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("===== ESP32-S3 Full Configuration =====");

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  Serial.println("--- Chip ---");
  Serial.printf("Chip Model       : %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision    : %d\n", ESP.getChipRevision());
  Serial.printf("CPU Cores        : %d\n", chip_info.cores);
  Serial.printf("CPU Frequency    : %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version      : %s\n", ESP.getSdkVersion());

  Serial.println();
  Serial.println("--- Flash ---");
  Serial.printf("Flash Size       : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Flash Speed      : %u MHz\n", ESP.getFlashChipSpeed() / 1000000);
  Serial.printf("Flash Mode       : %d (0=QIO 1=QOUT 2=DIO 3=DOUT 4=FAST_READ 5=SLOW_READ)\n", ESP.getFlashChipMode());

  Serial.println();
  Serial.println("--- PSRAM ---");
  if (psramFound()) {
    Serial.printf("PSRAM Size       : %u MB\n", ESP.getPsramSize() / (1024 * 1024));
    Serial.printf("Free PSRAM       : %u KB\n", ESP.getFreePsram() / 1024);
  } else {
    Serial.println("PSRAM NOT found / not enabled in build settings");
  }

  Serial.println();
  Serial.println("--- Memory ---");
  Serial.printf("Heap Size        : %u KB\n", ESP.getHeapSize() / 1024);
  Serial.printf("Free Heap        : %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("Min Free Heap    : %u KB\n", ESP.getMinFreeHeap() / 1024);

  Serial.println();
  Serial.println("--- Network MACs ---");
  Serial.printf("WiFi STA MAC     : %s\n", WiFi.macAddress().c_str());
  uint8_t btmac[6];
  esp_read_mac(btmac, ESP_MAC_BT);
  Serial.printf("Bluetooth MAC    : %02X:%02X:%02X:%02X:%02X:%02X\n",
                btmac[0], btmac[1], btmac[2], btmac[3], btmac[4], btmac[5]);

  Serial.println();
  Serial.println("--- Features ---");
  Serial.printf("WiFi 2.4GHz      : %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
  Serial.printf("BLE              : %s\n", (chip_info.features & CHIP_FEATURE_BLE) ? "Yes" : "No");
  Serial.printf("Embedded Flash   : %s\n", (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Yes" : "No");
  Serial.printf("Embedded PSRAM   : %s\n", (chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "Yes" : "No");

  Serial.println("=========================================");
}

void loop() {
  delay(10000);
}