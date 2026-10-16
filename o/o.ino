#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// =========================================================================
// DISPLAY & HARDWARE CONFIGURATION
// =========================================================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// I2C pins for ESP32-S3
#define SDA_PIN 8
#define SCL_PIN 9
#define OLED_ADDRESS 0x3C

// Hardware Buttons: ESP32-S3 onboard BOOT button is on GPIO 0
#define BOOT_BUTTON_PIN 0

// Onboard RGB LED (WS2812) on ESP32-S3
#ifndef RGB_LED_PIN
  #ifdef RGB_BUILTIN
    #define RGB_LED_PIN RGB_BUILTIN
  #else
    #define RGB_LED_PIN 48
  #endif
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =========================================================================
// 16x16 CUSTOM RETRO-FUTURISTIC BITMAP ICONS
// =========================================================================
// 1. Telemetry Gauge
const unsigned char PROGMEM icon_dashboard[] = {
  0b00001111, 0b11110000,
  0b00110000, 0b00001100,
  0b01000010, 0b01000010,
  0b10001000, 0b00010001,
  0b10010000, 0b00001001,
  0b10000001, 0b10000001,
  0b10000011, 0b11000001,
  0b10000111, 0b11100001,
  0b10000011, 0b11000001,
  0b10000001, 0b10000001,
  0b10010000, 0b00001001,
  0b10001000, 0b00010001,
  0b01000010, 0b01000010,
  0b00110000, 0b00001100,
  0b00001111, 0b11110000,
  0b00000000, 0b00000000
};

// 2. Wi-Fi Broadcast Waves
const unsigned char PROGMEM icon_wifi[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11111000,
  0b00110000, 0b00001100,
  0b01100111, 0b11100110,
  0b11001100, 0b00110011,
  0b00011000, 0b00011000,
  0b00000111, 0b11100000,
  0b00001100, 0b00110000,
  0b00011001, 0b10011000,
  0b00000011, 0b11000000,
  0b00000011, 0b11000000,
  0b00000000, 0b00000000,
  0b00000001, 0b10000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};

// 3. Audio / Synth Oscilloscope
const unsigned char PROGMEM icon_synth[] = {
  0b00000000, 0b00000000,
  0b00000001, 0b10000000,
  0b00000011, 0b11000000,
  0b00000011, 0b11000000,
  0b00000101, 0b10100000,
  0b00001001, 0b10010000,
  0b00010001, 0b10001000,
  0b00100001, 0b10000100,
  0b11000001, 0b10000011,
  0b00000001, 0b10000000,
  0b00000001, 0b10000000,
  0b10010010, 0b01001001,
  0b10110110, 0b01101101,
  0b11111111, 0b11111111,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};

// 4. Warp Spaceship / Cosmos
const unsigned char PROGMEM icon_starfield[] = {
  0b00000001, 0b10000000,
  0b00000011, 0b11000000,
  0b00000111, 0b11100000,
  0b00000110, 0b01100000,
  0b00001110, 0b01110000,
  0b00001111, 0b11110000,
  0b00011111, 0b11111000,
  0b00111111, 0b11111100,
  0b01110111, 0b11101110,
  0b11100111, 0b11100111,
  0b11000111, 0b11100011,
  0b00000101, 0b10100000,
  0b00000100, 0b00100000,
  0b00000010, 0b01000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000
};

// 5. Settings / Mechanical Cog
const unsigned char PROGMEM icon_settings[] = {
  0b00000110, 0b01100000,
  0b00000110, 0b01100000,
  0b00110011, 0b11001100,
  0b01111111, 0b11111110,
  0b01101100, 0b00110110,
  0b00001000, 0b00010000,
  0b11011000, 0b00011011,
  0b11011000, 0b00011011,
  0b00001000, 0b00010000,
  0b01101100, 0b00110110,
  0b01111111, 0b11111110,
  0b00110011, 0b11001100,
  0b00000110, 0b01100000,
  0b00000110, 0b01100000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};

// 6. System Info / Microchip
const unsigned char PROGMEM icon_info[] = {
  0b00001010, 0b10100000,
  0b01111111, 0b11111110,
  0b01000000, 0b00000010,
  0b11011111, 0b11111011,
  0b01010000, 0b00001010,
  0b11010111, 0b11101011,
  0b01010100, 0b00101010,
  0b11010101, 0b10101011,
  0b01010100, 0b00101010,
  0b11010111, 0b11101011,
  0b01010000, 0b00001010,
  0b11011111, 0b11111011,
  0b01000000, 0b00000010,
  0b01111111, 0b11111110,
  0b00001010, 0b10100000,
  0b00000000, 0b00000000
};

// =========================================================================
// DATA STRUCTURES
// =========================================================================
struct MenuItem {
  const char* label;
  const unsigned char* icon;
};

const int NUM_ITEMS = 6;
const MenuItem menuItems[NUM_ITEMS] = {
  {"TELEMETRY",    icon_dashboard},
  {"WIFI RADAR",   icon_wifi},
  {"AUDIO SYNTH",  icon_synth},
  {"3D STARFIELD", icon_starfield},
  {"SETTINGS",     icon_settings},
  {"SYSTEM INFO",  icon_info}
};

enum AppState {
  STATE_MENU = 0,
  STATE_DASHBOARD,
  STATE_WIFI,
  STATE_SYNTH,
  STATE_STARFIELD,
  STATE_SETTINGS,
  STATE_INFO
};

AppState currentState = STATE_MENU;
int selectedIndex = 0;
int topVisibleIndex = 0;

// Floating selector physics (momentum easing)
float currentPillY = 14.0f;
float targetPillY = 14.0f;

// Settings states
bool isInverted = false;
bool isDimmed = false;
bool isRgbOn = false;
bool autoDemo = false;
int settingsSubIndex = 0;

// Global animation tickers
unsigned long frameCount = 0;
unsigned long lastDemoSwitch = 0;
unsigned long lastButtonPressTime = 0;
bool buttonPrevState = HIGH;

// =========================================================================
// 3D STARFIELD
// =========================================================================
#define NUM_STARS 45
struct Star { float x, y, z; };
Star stars[NUM_STARS];

void initStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = (float)(random(-100, 100));
    stars[i].y = (float)(random(-50, 50));
    stars[i].z = (float)(random(1, 100));
  }
}

// Wi-Fi scanner data
int scannedNetworksCount = -1;
unsigned long lastScanRequest = 0;
float radarAngle = 0.0f;

// Audio synth data
#define NUM_BARS 16
int eqHeights[NUM_BARS];
int eqPeaks[NUM_BARS];

// Forward declaration
void navigateTo(AppState newState);

// =========================================================================
// CINEMATIC SCREEN TRANSITIONS
// =========================================================================
// Plays an aperture zoom & scanline shutter transition
void playTransition(bool entering, int originY = 32) {
  if (entering) {
    // 1. Quick tactile pulse flash on origin
    display.fillRoundRect(2, originY, 118, 15, 3, SSD1306_INVERSE);
    display.display();
    delay(30);

    // 2. Expanding Cyber Aperture with corner reticles [ ]
    for (int step = 1; step <= 6; step++) {
      float t = (float)step / 6.0f;
      float ease = t * (2.0f - t); // Ease-out quad

      int x = (int)((1.0f - ease) * 4.0f);
      int y = (int)((1.0f - ease) * originY);
      int w = (int)(ease * 128.0f + (1.0f - ease) * 118.0f);
      int h = (int)(ease * 64.0f + (1.0f - ease) * 15.0f);

      display.clearDisplay();

      // Rounded expanding viewport frame
      display.drawRoundRect(x, y, w, h, 3, SSD1306_WHITE);

      // Sci-fi corner brackets
      int bSize = 6;
      display.drawLine(x, y, x + bSize, y, SSD1306_WHITE);
      display.drawLine(x, y, x, y + bSize, SSD1306_WHITE);
      display.drawLine(x + w - 1, y, x + w - 1 - bSize, y, SSD1306_WHITE);
      display.drawLine(x + w - 1, y, x + w - 1, y + bSize, SSD1306_WHITE);
      display.drawLine(x, y + h - 1, x + bSize, y + h - 1, SSD1306_WHITE);
      display.drawLine(x, y + h - 1, x, y + h - 1 - bSize, SSD1306_WHITE);
      display.drawLine(x + w - 1, y + h - 1, x + w - 1 - bSize, y + h - 1, SSD1306_WHITE);
      display.drawLine(x + w - 1, y + h - 1, x + w - 1, y + h - 1 - bSize, SSD1306_WHITE);

      // Fast laser scanline sweeping across
      int scanY = (int)(ease * 63.0f);
      display.drawFastHLine(0, scanY, 128, SSD1306_WHITE);

      display.display();
      delay(18);
    }
  } else {
    // CONTRACTING COLLAPSE (Return to Menu)
    for (int step = 1; step <= 5; step++) {
      float t = (float)step / 5.0f;
      float ease = t * t; // Ease-in quad

      int w = (int)((1.0f - ease) * 128.0f + ease * 30.0f);
      int h = (int)((1.0f - ease) * 64.0f + ease * 8.0f);
      int x = (128 - w) / 2;
      int y = (64 - h) / 2;

      display.clearDisplay();
      display.drawRoundRect(x, y, w, h, 2, SSD1306_WHITE);

      // Center crosshair blip
      display.drawFastHLine(60, 32, 8, SSD1306_WHITE);
      display.drawFastVLine(64, 28, 8, SSD1306_WHITE);

      display.display();
      delay(18);
    }
  }
}

// State navigation with cinematic transition
void navigateTo(AppState newState) {
  if (newState != currentState) {
    if (newState == STATE_MENU) {
      playTransition(false); // Contract back to menu
    } else {
      playTransition(true, (int)(currentPillY + 0.5f)); // Expand from pill
    }
    currentState = newState;
  }
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);

  // Setup hardware BOOT button (GPIO 0)
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Initialize I2C with ESP32-S3 pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("❌ OLED initialization failed!");
    while (true) delay(100);
  }

  // Ensure Onboard RGB LED starts OFF
  #ifdef RGB_BUILTIN
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
  #endif
  rgbLedWrite(RGB_LED_PIN, 0, 0, 0);

  // Initialize Wi-Fi in Station mode for scanning
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  initStarfield();

  // Print aesthetic Serial instructions
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║          ESP32-S3 AESTHETIC OLED OS v3.0               ║");
  Serial.println("╠════════════════════════════════════════════════════════╣");
  Serial.println("║  🎮 CONTROLS (Physical BOOT button on board GPIO 0):   ║");
  Serial.println("║     • Short Press (<400ms) : Navigate / Move Down      ║");
  Serial.println("║     • Long Press (>400ms)  : Select / Enter / Back     ║");
  Serial.println("║                                                        ║");
  Serial.println("║  ⌨️  SERIAL CONTROLS (115200 Baud):                     ║");
  Serial.println("║     • [w] / [s] or [k] / [j] : Up / Down               ║");
  Serial.println("║     • [Enter] or [e] or [Space]: Select / Open         ║");
  Serial.println("║     • [b] or [q]             : Back to Menu            ║");
  Serial.println("║     • [i]                    : Quick Invert (Dark/Lt)  ║");
  Serial.println("║     • [d]                    : Quick Dim Toggle        ║");
  Serial.println("║     • [r]                    : Quick RGB LED Toggle    ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");

  // ----------------------------------------------------
  // Cyberpunk Boot Animation
  // ----------------------------------------------------
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(22, 16);
  display.print("NEO-OS v3.0");
  display.setCursor(18, 28);
  display.print("ESP32-S3 CORE");

  display.drawFastHLine(12, 40, 104, SSD1306_WHITE);
  for (int p = 0; p <= 100; p += 10) {
    display.fillRect(14, 43, p, 4, SSD1306_WHITE);
    display.display();
    delay(25);
  }
  delay(250);
}

// =========================================================================
// DRAW GLOBAL STATUS BAR (Top 12 Pixels)
// =========================================================================
void drawStatusBar(const char* title, bool showBackHint = false) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (showBackHint) {
    display.setCursor(1, 1);
    display.print("<");
    display.setCursor(10, 1);
    display.print(title);
  } else {
    display.setCursor(2, 1);
    display.print("SYS//");
    display.print(title);
  }

  // Active pulsing heartbeat
  if ((frameCount / 6) % 2 == 0) {
    display.fillCircle(82, 4, 2, SSD1306_WHITE);
  } else {
    display.drawCircle(82, 4, 2, SSD1306_WHITE);
  }

  // Mini Battery / Power Badge
  display.drawRect(95, 1, 18, 8, SSD1306_WHITE);
  display.drawFastVLine(113, 3, 4, SSD1306_WHITE);
  display.fillRect(97, 3, 13, 4, SSD1306_WHITE);

  // Invert / Dim indicator dots
  if (isInverted) display.drawPixel(120, 2, SSD1306_WHITE);
  if (isDimmed)   display.drawPixel(124, 2, SSD1306_WHITE);

  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);
}

// =========================================================================
// SCENE 0: MAIN MENU (Smooth Gliding Floating Inverted Pill)
// =========================================================================
void renderMainMenu() {
  drawStatusBar("MENU");

  // Keep topVisibleIndex adjusted
  if (selectedIndex < topVisibleIndex) {
    topVisibleIndex = selectedIndex;
  } else if (selectedIndex >= topVisibleIndex + 3) {
    topVisibleIndex = selectedIndex - 2;
  }

  // Calculate target position for animated floating pill
  targetPillY = 14.0f + (selectedIndex - topVisibleIndex) * 16.5f;
  currentPillY += (targetPillY - currentPillY) * 0.38f; // Kinetic momentum easing

  // Draw 3 visible items
  for (int i = 0; i < 3; i++) {
    int itemIdx = topVisibleIndex + i;
    if (itemIdx >= NUM_ITEMS) break;

    int itemY = 14 + i * 17;
    bool isSelected = (itemIdx == selectedIndex);

    if (isSelected) {
      // Draw smooth floating white capsule / pill
      int pillY = (int)(currentPillY + 0.5f);
      display.fillRoundRect(2, pillY, 118, 15, 3, SSD1306_WHITE);

      // Icon in black (inverted)
      display.drawBitmap(6, pillY, menuItems[itemIdx].icon, 16, 16, SSD1306_BLACK);

      // Label in black
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.setCursor(26, pillY + 4);
      display.print(menuItems[itemIdx].label);

      // Right arrow badge in black
      display.setCursor(110, pillY + 4);
      display.print(">");
    } else {
      // Unselected item: icon & label in white
      display.drawBitmap(6, itemY, menuItems[itemIdx].icon, 16, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(26, itemY + 4);
      display.print(menuItems[itemIdx].label);
    }
  }

  // Sleek Right-Edge Scrollbar
  display.drawFastVLine(124, 14, 48, SSD1306_WHITE);
  int thumbH = 14;
  int thumbY = 14 + (selectedIndex * (48 - thumbH)) / (NUM_ITEMS - 1);
  display.fillRoundRect(123, thumbY, 3, thumbH, 1, SSD1306_WHITE);
}

// =========================================================================
// SCENE 1: TELEMETRY DASHBOARD
// =========================================================================
void renderTelemetryScreen() {
  drawStatusBar("TELEMETRY", true);

  // Left Circular Dial Gauge (x=22, y=36, radius=16)
  display.drawCircle(22, 36, 17, SSD1306_WHITE);
  display.drawCircle(22, 36, 16, SSD1306_WHITE);
  display.drawCircle(22, 36, 3, SSD1306_WHITE);

  // Animated sweep needle
  float angle = (frameCount * 0.08f);
  int nx = 22 + (int)(13.0f * cos(angle));
  int ny = 36 + (int)(13.0f * sin(angle));
  display.drawLine(22, 36, nx, ny, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12, 55);
  display.print("CORE");

  // Divider
  display.drawFastVLine(46, 13, 50, SSD1306_WHITE);

  // Right Telemetry Metrics
  display.setCursor(50, 15);
  display.print("RAM :");
  display.setCursor(82, 15);
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB");

  display.setCursor(50, 26);
  display.print("CPU :");
  display.setCursor(82, 26);
  display.print(ESP.getCpuFreqMHz());
  display.print("MHz");

  display.setCursor(50, 37);
  display.print("UP  :");
  unsigned long s = millis() / 1000;
  display.printf("%02lu:%02lu", (s / 60) % 60, s % 60);

  // Bottom mini progress bar
  int ramPct = map(ESP.getFreeHeap(), 0, 327680, 0, 72);
  display.drawRect(50, 50, 74, 8, SSD1306_WHITE);
  display.fillRect(52, 52, constrain(ramPct, 2, 70), 4, SSD1306_WHITE);
}

// =========================================================================
// SCENE 2: WI-FI RADAR SCANNER
// =========================================================================
void renderWifiScreen() {
  drawStatusBar("WIFI RADAR", true);

  if (millis() - lastScanRequest > 8000) {
    WiFi.scanNetworks(true); // Non-blocking async scan
    lastScanRequest = millis();
  }

  int scanStatus = WiFi.scanComplete();
  if (scanStatus >= 0) {
    scannedNetworksCount = scanStatus;
  }

  // Left Radar Scope
  display.drawCircle(24, 38, 18, SSD1306_WHITE);
  display.drawCircle(24, 38, 10, SSD1306_WHITE);
  display.drawFastHLine(6, 38, 36, SSD1306_WHITE);
  display.drawFastVLine(24, 20, 36, SSD1306_WHITE);

  // Animated sweeping radar beam
  radarAngle += 0.12f;
  int rx = 24 + (int)(18.0f * cos(radarAngle));
  int ry = 38 + (int)(18.0f * sin(radarAngle));
  display.drawLine(24, 38, rx, ry, SSD1306_WHITE);

  // Blips
  display.fillCircle(18, 30, 1, SSD1306_WHITE);
  display.fillCircle(33, 44, 1, SSD1306_WHITE);
  display.fillCircle(30, 26, 1, SSD1306_WHITE);

  // Divider
  display.drawFastVLine(46, 13, 50, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (scannedNetworksCount < 0) {
    display.setCursor(50, 20);
    display.print("Scanning");
    display.setCursor(50, 34);
    display.print("2.4 GHz...");
    int dots = (frameCount / 8) % 4;
    for (int d = 0; d < dots; d++) display.print(".");
  } else if (scannedNetworksCount == 0) {
    display.setCursor(50, 26);
    display.print("No AP Found");
  } else {
    for (int i = 0; i < min(3, scannedNetworksCount); i++) {
      int y = 14 + i * 16;
      String ssid = WiFi.SSID(i);
      if (ssid.length() > 7) ssid = ssid.substring(0, 7);
      display.setCursor(50, y);
      display.print(ssid);

      // Signal RSSI bars
      int rssi = WiFi.RSSI(i);
      int bars = map(constrain(rssi, -100, -50), -100, -50, 1, 4);
      for (int b = 0; b < bars; b++) {
        display.fillRect(108 + b * 4, y + 6 - (b * 2), 2, 2 + (b * 2), SSD1306_WHITE);
      }
    }
  }

  display.setCursor(1, 56);
  if (scannedNetworksCount >= 0) display.printf("%d APs", scannedNetworksCount);
}

// =========================================================================
// SCENE 3: AUDIO SYNTH & OSCILLOSCOPE
// =========================================================================
void renderSynthScreen() {
  drawStatusBar("AUDIO SYNTH", true);

  // Top Oscilloscope Sine Wave (y = 13..34)
  int prevY = 24;
  for (int x = 0; x < 128; x++) {
    float a1 = (x * 0.12f) + (frameCount * 0.20f);
    float a2 = (x * 0.05f) - (frameCount * 0.15f);
    int y = 23 + (int)(8.0f * sin(a1) + 4.0f * cos(a2));

    if (x > 0) display.drawLine(x - 1, prevY, x, y, SSD1306_WHITE);
    prevY = y;
  }

  display.drawFastHLine(0, 36, 128, SSD1306_WHITE);

  // Bottom 16-Band Equalizer (y = 38..63)
  const int barW = 6;
  const int gap = 2;
  for (int b = 0; b < NUM_BARS; b++) {
    int targetH = 3 + (int)(22.0f * (0.5f + 0.5f * sin((frameCount * 0.25f) + b * 0.6f)));
    if (targetH > 24) targetH = 24;

    if (targetH > eqHeights[b]) eqHeights[b] = targetH;
    else if (eqHeights[b] > 1) eqHeights[b]--;

    if (eqHeights[b] >= eqPeaks[b]) {
      eqPeaks[b] = eqHeights[b];
    } else if (eqPeaks[b] > 0 && (frameCount % 3 == 0)) {
      eqPeaks[b]--;
    }

    int bx = b * (barW + gap);
    int by = 63 - eqHeights[b];
    display.fillRect(bx, by, barW, eqHeights[b], SSD1306_WHITE);

    int peakY = 63 - eqPeaks[b] - 1;
    if (peakY >= 37 && peakY < 63) {
      display.drawFastHLine(bx, peakY, barW, SSD1306_WHITE);
    }
  }
}

// =========================================================================
// SCENE 4: 3D CYBER STARFIELD (WARP SPEED)
// =========================================================================
void renderStarfieldScreen() {
  drawStatusBar("3D WARP", true);

  const float centerX = 64.0f;
  const float centerY = 37.0f;
  const float speed = 2.5f;

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].z -= speed;

    if (stars[i].z <= 1.0f) {
      stars[i].x = (float)(random(-100, 100));
      stars[i].y = (float)(random(-50, 50));
      stars[i].z = 100.0f;
    }

    float k = 50.0f / stars[i].z;
    int px = (int)(stars[i].x * k + centerX);
    int py = (int)(stars[i].y * k + centerY);

    if (px >= 0 && px < 128 && py >= 12 && py < 64) {
      if (stars[i].z < 35.0f) {
        display.fillRect(px, py, 2, 2, SSD1306_WHITE);
      } else {
        display.drawPixel(px, py, SSD1306_WHITE);
      }
    }
  }

  // HUD Crosshairs
  display.drawCircle((int)centerX, (int)centerY, 8, SSD1306_WHITE);
  display.drawFastHLine(58, (int)centerY, 12, SSD1306_WHITE);
  display.drawFastVLine((int)centerX, 31, 12, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 54);
  display.print("WARP 9.8");
  display.setCursor(80, 54);
  display.print("VEL: c");
}

// =========================================================================
// SCENE 5: SETTINGS & LIVE CONTROLS
// =========================================================================
void renderSettingsScreen() {
  drawStatusBar("SETTINGS", true);

  const char* settingsNames[] = {"THEME", "BRIGHT", "RGB LED", "AUTO DEMO"};
  const char* settingsValues[] = {
    isInverted ? "LIGHT" : "DARK",
    isDimmed ? "DIM" : "FULL",
    isRgbOn ? "ON" : "OFF",
    autoDemo ? "ON" : "OFF"
  };

  for (int i = 0; i < 4; i++) {
    int y = 14 + i * 12;
    bool isSel = (i == settingsSubIndex);

    if (isSel) {
      display.fillRect(2, y, 124, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setCursor(6, y + 2);
    display.print(settingsNames[i]);

    display.setCursor(86, y + 2);
    display.print("[");
    display.print(settingsValues[i]);
    display.print("]");
  }

  display.setTextColor(SSD1306_WHITE);
  display.drawFastHLine(0, 63, 128, SSD1306_WHITE);
}

// =========================================================================
// SCENE 6: SYSTEM SPECS
// =========================================================================
void renderInfoScreen() {
  drawStatusBar("SYSTEM INFO", true);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(4, 15);
  display.print("CHIP : ESP32-S3 Rev ");
  display.print(ESP.getChipRevision());

  display.setCursor(4, 26);
  display.print("CORES: 2 @ 240 MHz");

  display.setCursor(4, 37);
  display.print("FLASH: 16MB | 8MB PS");

  display.setCursor(4, 48);
  display.print("MAC  : ");
  String mac = WiFi.macAddress();
  display.print(mac.substring(9));

  display.drawFastHLine(0, 58, 128, SSD1306_WHITE);
  display.setCursor(4, 57);
  display.print("STATUS: 100% OPTIMAL");
}

// =========================================================================
// INPUT PROCESSING (Physical BOOT Button + Serial Monitor)
// =========================================================================
void toggleCurrentSetting() {
  switch (settingsSubIndex) {
    case 0:
      isInverted = !isInverted;
      display.invertDisplay(isInverted);
      break;
    case 1:
      isDimmed = !isDimmed;
      display.dim(isDimmed);
      break;
    case 2:
      isRgbOn = !isRgbOn;
      if (isRgbOn) rgbLedWrite(RGB_LED_PIN, 30, 30, 30);
      else rgbLedWrite(RGB_LED_PIN, 0, 0, 0);
      break;
    case 3:
      autoDemo = !autoDemo;
      lastDemoSwitch = millis();
      break;
  }
}

void handleInputs() {
  // 1. Physical BOOT Button (GPIO 0) handling with short / long press detection
  bool btnState = digitalRead(BOOT_BUTTON_PIN);

  if (btnState == LOW && buttonPrevState == HIGH) {
    lastButtonPressTime = millis();
  } else if (btnState == HIGH && buttonPrevState == LOW) {
    unsigned long duration = millis() - lastButtonPressTime;

    if (duration > 450) {
      // LONG PRESS (>450ms) -> Enter / Select / Back
      if (currentState == STATE_MENU) {
        navigateTo((AppState)(selectedIndex + 1));
        Serial.print("▶ Entered: ");
        Serial.println(menuItems[selectedIndex].label);
      } else if (currentState == STATE_SETTINGS) {
        toggleCurrentSetting();
      } else {
        navigateTo(STATE_MENU);
        Serial.println("◀ Returned to Main Menu");
      }
    } else if (duration > 30) {
      // SHORT PRESS -> Navigate down
      if (currentState == STATE_MENU) {
        selectedIndex = (selectedIndex + 1) % NUM_ITEMS;
      } else if (currentState == STATE_SETTINGS) {
        settingsSubIndex = (settingsSubIndex + 1) % 4;
      } else {
        navigateTo(STATE_MENU);
      }
    }
  }
  buttonPrevState = btnState;

  // 2. Serial Monitor keyboard commands
  if (Serial.available()) {
    char ch = Serial.read();

    // Navigate UP
    if (ch == 'w' || ch == 'W' || ch == 'k' || ch == 'K' || ch == '8') {
      if (currentState == STATE_MENU) {
        selectedIndex = (selectedIndex - 1 + NUM_ITEMS) % NUM_ITEMS;
      } else if (currentState == STATE_SETTINGS) {
        settingsSubIndex = (settingsSubIndex - 1 + 4) % 4;
      }
    }
    // Navigate DOWN
    else if (ch == 's' || ch == 'S' || ch == 'j' || ch == 'J' || ch == '2') {
      if (currentState == STATE_MENU) {
        selectedIndex = (selectedIndex + 1) % NUM_ITEMS;
      } else if (currentState == STATE_SETTINGS) {
        settingsSubIndex = (settingsSubIndex + 1) % 4;
      }
    }
    // SELECT / ENTER
    else if (ch == '\r' || ch == '\n' || ch == 'e' || ch == 'E' || ch == ' ') {
      if (currentState == STATE_MENU) {
        navigateTo((AppState)(selectedIndex + 1));
        Serial.print("▶ Opened: ");
        Serial.println(menuItems[selectedIndex].label);
      } else if (currentState == STATE_SETTINGS) {
        toggleCurrentSetting();
      }
    }
    // BACK TO MENU
    else if (ch == 'b' || ch == 'B' || ch == 'q' || ch == 'Q' || ch == 27) {
      navigateTo(STATE_MENU);
      Serial.println("◀ Back to Menu");
    }
    // Quick Invert toggle
    else if (ch == 'i' || ch == 'I') {
      isInverted = !isInverted;
      display.invertDisplay(isInverted);
      Serial.println(isInverted ? "🎨 Theme: LIGHT" : "🎨 Theme: DARK");
    }
    // Quick Dim toggle
    else if (ch == 'd' || ch == 'D') {
      isDimmed = !isDimmed;
      display.dim(isDimmed);
      Serial.println(isDimmed ? "💡 Dim: ON" : "💡 Dim: OFF");
    }
    // Quick RGB toggle
    else if (ch == 'r' || ch == 'R') {
      isRgbOn = !isRgbOn;
      if (isRgbOn) rgbLedWrite(RGB_LED_PIN, 30, 30, 30);
      else rgbLedWrite(RGB_LED_PIN, 0, 0, 0);
      Serial.println(isRgbOn ? "💡 RGB LED: ON" : "💡 RGB LED: OFF");
    }
    // Quick Auto-Demo toggle
    else if (ch == 'a' || ch == 'A') {
      autoDemo = !autoDemo;
      lastDemoSwitch = millis();
      Serial.println(autoDemo ? "🔄 Auto-Demo: ACTIVE" : "🔄 Auto-Demo: DISABLED");
    }
  }

  // 3. Auto-Demo Showcase Mode (if enabled)
  if (autoDemo && (millis() - lastDemoSwitch > 6000)) {
    lastDemoSwitch = millis();
    if (currentState == STATE_MENU) {
      selectedIndex = (selectedIndex + 1) % NUM_ITEMS;
      navigateTo((AppState)(selectedIndex + 1));
    } else {
      navigateTo(STATE_MENU);
    }
  }
}

// =========================================================================
// MAIN LOOP
// =========================================================================
void loop() {
  handleInputs();

  display.clearDisplay();

  switch (currentState) {
    case STATE_MENU:
      renderMainMenu();
      break;
    case STATE_DASHBOARD:
      renderTelemetryScreen();
      break;
    case STATE_WIFI:
      renderWifiScreen();
      break;
    case STATE_SYNTH:
      renderSynthScreen();
      break;
    case STATE_STARFIELD:
      renderStarfieldScreen();
      break;
    case STATE_SETTINGS:
      renderSettingsScreen();
      break;
    case STATE_INFO:
      renderInfoScreen();
      break;
  }

  display.display();
  frameCount++;

  // Smooth ~30 FPS frame rate
  delay(30);
}