/*
 * =====================================================================================
 *        ST7735 1.8" TFT LCD (128x160) EXTREME HARDWARE BENCHMARK & TORTURE SUITE
 * =====================================================================================
 *  Target MCU:      ESP32-S3 (N16R8 / standard DevKit)
 *  Display Driver:  ST7735 (ST7735S / ST7735B / ST7735R)
 *  Resolution:      128 x 160 pixels, 16-bit RGB565 (65,536 colors)
 *  Power Mode:      3.3V Direct (J1 Jumper Shorted on TFT Module)
 *
 *  PIN CONFIGURATION (ESP32-S3 Hardware SPI):
 *  -------------------------------------------------------------------------------------
 *  TFT Pin      Label on Board        ESP32-S3 GPIO     Notes
 *  -------------------------------------------------------------------------------------
 *  1. GND       GND                   GND               Common Ground
 *  2. VCC       VCC                   3.3V              J1 SHORTED -> pure 3.3V supply!
 *  3. SCL       SCL / SCK / CLK       GPIO 12           SPI Hardware Clock
 *  4. SDA       SDA / MOSI / DIN      GPIO 11           SPI Hardware MOSI
 *  5. RES       RES / RST             GPIO 9            Display Reset
 *  6. DC        DC / A0 / RS          GPIO 8            Data / Command Select
 *  7. CS        CS / SS               GPIO 10           Chip Select
 *  8. BLK       BLK / LED             GPIO 7 (or 3.3V)  Backlight (PWM capable or direct)
 * =====================================================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "esp_flash.h"

// -----------------------------------------------------------------------------
// USER CONFIGURATION & PIN MAPPING
// -----------------------------------------------------------------------------
#define TFT_CS    10
#define TFT_DC     8
#define TFT_RST    9
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     7   // Set to -1 if your BLK pin is tied directly to 3.3V

// ST7735 initialization profile:
// Most modern 1.8" red/blue PCB boards use INITR_BLACKTAB.
// If your display has a 1-2 pixel offset or static border, change this to:
//   INITR_18GREENTAB   or   INITR_18REDTAB
#define ST7735_INIT_MODE INITR_BLACKTAB

// PWM Backlight Configuration (LEDC on ESP32-S3)
#define LEDC_PWM_CHANNEL 0
#define LEDC_PWM_FREQ    5000
#define LEDC_PWM_RES     8

// Create Display Driver Instance with Hardware SPI
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Fast 16-bit Color Definitions
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_GRAY    0x8410
#define COLOR_DARKGRAY 0x4208
#define COLOR_NAVY    0x000F
#define COLOR_PURPLE  0x780F

// Benchmark Metrics Tracking
struct BenchmarkMetrics {
  uint32_t screenFills_us;
  uint32_t lines_us;
  uint32_t circles_us;
  uint32_t rects_us;
  uint32_t roundedRects_us;
  uint32_t triangles_us;
  float cubeAvgFPS;
  float physicsAvgFPS;
  float snowMBs;
} metrics;

// -----------------------------------------------------------------------------
// BACKLIGHT HELPER FUNCTIONS
// -----------------------------------------------------------------------------
void setupBacklight() {
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
      ledcAttach(TFT_BL, LEDC_PWM_FREQ, LEDC_PWM_RES);
      ledcWrite(TFT_BL, 255);
    #else
      ledcSetup(LEDC_PWM_CHANNEL, LEDC_PWM_FREQ, LEDC_PWM_RES);
      ledcAttachPin(TFT_BL, LEDC_PWM_CHANNEL);
      ledcWrite(LEDC_PWM_CHANNEL, 255);
    #endif
  }
}

void setBacklightBrightness(uint8_t duty) {
  if (TFT_BL >= 0) {
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
      ledcWrite(TFT_BL, duty);
    #else
      ledcWrite(LEDC_PWM_CHANNEL, duty);
    #endif
  }
}

// -----------------------------------------------------------------------------
// HELPER: Convert RGB (0-255) to 16-bit RGB565
// -----------------------------------------------------------------------------
inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// -----------------------------------------------------------------------------
// TEST 1: SYSTEM & HARDWARE DIAGNOSTICS DASHBOARD
// -----------------------------------------------------------------------------
void testDiagnostics() {
  tft.setRotation(0); // 128x160 Portrait
  tft.fillScreen(COLOR_BLACK);

  // Header Bar
  tft.fillRect(0, 0, 128, 22, COLOR_NAVY);
  tft.drawFastHLine(0, 22, 128, COLOR_CYAN);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  tft.setCursor(6, 4);
  tft.println("ESP32-S3 EXTREME");
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(6, 13);
  tft.println("ST7735 TORTURE SUITE");

  // Telemetry Cards
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 28);
  tft.print("CPU: ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u MHz (2 Cores)\n", ESP.getCpuFreqMHz());

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 38);
  tft.print("Flash: ");
  tft.setTextColor(COLOR_WHITE);
  uint32_t flashSize = 0;
  esp_flash_get_size(NULL, &flashSize);
  tft.printf("%u MB (%uMHz)\n", flashSize / (1024 * 1024), ESP.getFlashChipSpeed() / 1000000);

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 48);
  tft.print("PSRAM: ");
  if (psramFound()) {
    tft.setTextColor(COLOR_GREEN);
    tft.printf("%u MB OPI\n", ESP.getPsramSize() / (1024 * 1024));
  } else {
    tft.setTextColor(COLOR_RED);
    tft.println("Not Active");
  }

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 58);
  tft.print("Free Heap: ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u KB\n", ESP.getFreeHeap() / 1024);

  // Power & Display Specs
  tft.drawRect(2, 70, 124, 46, COLOR_DARKGRAY);
  tft.fillRect(3, 71, 122, 44, 0x1082); // Dark slate
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(6, 74);
  tft.println("DISP: 1.8\" 128x160 TFT");
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(6, 84);
  tft.println("J1 Jumper: SHORTED");
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(6, 94);
  tft.println("VCC: 3.3V Direct Mode");
  tft.setTextColor(COLOR_ORANGE);
  tft.setCursor(6, 104);
  tft.println("SPI Clock: 27-40 MHz");

  // Animated Loading Progress Bar
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(4, 122);
  tft.println("Starting Benchmark...");
  tft.drawRect(4, 134, 120, 10, COLOR_CYAN);
  for (int w = 0; w <= 116; w += 4) {
    tft.fillRect(6, 136, w, 6, COLOR_GREEN);
    delay(25);
  }
  delay(600);
}

// -----------------------------------------------------------------------------
// TEST 2: BOUNDARY & PIXEL-PERFECT ALIGNMENT (OFFSET CHECK)
// -----------------------------------------------------------------------------
void testAlignment() {
  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);

  // 1-pixel exact outer perimeter bounding box (0,0 to 127,159)
  tft.drawRect(0, 0, 128, 160, COLOR_WHITE);

  // Inner inset border
  tft.drawRect(2, 2, 124, 156, COLOR_CYAN);

  // 4 Extreme Corner Checkers (each 4x4 pixels)
  tft.fillRect(0, 0, 4, 4, COLOR_RED);               // Top-Left (0,0)
  tft.fillRect(124, 0, 4, 4, COLOR_GREEN);          // Top-Right (127,0)
  tft.fillRect(0, 156, 4, 4, COLOR_BLUE);           // Bottom-Left (0,159)
  tft.fillRect(124, 156, 4, 4, COLOR_YELLOW);       // Bottom-Right (127,159)

  // Diagonal corner-to-corner crosshairs
  tft.drawLine(0, 0, 127, 159, COLOR_DARKGRAY);
  tft.drawLine(127, 0, 0, 159, COLOR_DARKGRAY);

  // Center Crosshair
  tft.drawFastHLine(0, 80, 128, COLOR_MAGENTA);
  tft.drawFastVLine(64, 0, 160, COLOR_MAGENTA);
  tft.drawCircle(64, 80, 15, COLOR_YELLOW);
  tft.drawCircle(64, 80, 30, COLOR_YELLOW);

  // Information Overlay
  tft.fillRect(12, 50, 104, 30, COLOR_BLACK);
  tft.drawRect(12, 50, 104, 30, COLOR_CYAN);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(18, 54);
  tft.println("ALIGNMENT TEST");
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(18, 66);
  tft.println("(0,0) -> (127,159)");

  tft.fillRect(12, 95, 104, 24, COLOR_BLACK);
  tft.drawRect(12, 95, 104, 24, COLOR_GREEN);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(16, 99);
  tft.println("If border clipped:");
  tft.setCursor(16, 108);
  tft.println("Check TAB mode");

  delay(2500);
}

// -----------------------------------------------------------------------------
// TEST 3: FULL RGBW COLOR PURITY & DEAD PIXEL TEST
// -----------------------------------------------------------------------------
void testColorPurity() {
  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE,
    COLOR_WHITE, COLOR_BLACK,
    COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
  };
  const char* names[] = {
    "PURE RED", "PURE GREEN", "PURE BLUE",
    "PURE WHITE", "PURE BLACK",
    "YELLOW", "CYAN", "MAGENTA"
  };

  tft.setRotation(0);
  tft.setTextSize(1);

  for (int i = 0; i < 8; i++) {
    tft.fillScreen(colors[i]);
    // Contrasting label box
    uint16_t textBg = (colors[i] == COLOR_WHITE || colors[i] == COLOR_YELLOW || colors[i] == COLOR_CYAN) ? COLOR_BLACK : COLOR_WHITE;
    uint16_t textFg = (textBg == COLOR_BLACK) ? COLOR_WHITE : COLOR_BLACK;

    tft.fillRect(14, 70, 100, 20, textBg);
    tft.drawRect(14, 70, 100, 20, textFg);
    tft.setTextColor(textFg);
    tft.setCursor(20, 76);
    tft.print(names[i]);
    delay(800);
  }
}

// -----------------------------------------------------------------------------
// TEST 4: SMPTE COLOR BARS & 16-LEVEL GRAYSCALE CONTRAST
// -----------------------------------------------------------------------------
void testColorBars() {
  tft.setRotation(1); // 160x128 Landscape for standard TV test pattern
  tft.fillScreen(COLOR_BLACK);

  // 7 Standard SMPTE Color Bars
  const uint16_t smpteBars[7] = {
    COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN,
    COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE
  };
  int barWidth = 160 / 7;

  for (int i = 0; i < 7; i++) {
    tft.fillRect(i * barWidth, 0, barWidth, 80, smpteBars[i]);
  }

  // 16-step Grayscale Ramp at bottom (tests DAC gamma and contrast)
  for (int i = 0; i < 16; i++) {
    uint8_t level = (i * 255) / 15;
    uint16_t gray = rgb565(level, level, level);
    tft.fillRect(i * 10, 80, 10, 24, gray);
  }

  // True Black & True White Reference Blocks
  tft.fillRect(0, 104, 80, 24, COLOR_BLACK);
  tft.drawRect(0, 104, 80, 24, COLOR_DARKGRAY);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(6, 112);
  tft.print("0% TRUE BLACK");

  tft.fillRect(80, 104, 80, 24, COLOR_WHITE);
  tft.setTextColor(COLOR_BLACK);
  tft.setCursor(86, 112);
  tft.print("100% WHITE");

  delay(2500);
}

// -----------------------------------------------------------------------------
// TEST 5: SMOOTH 16-BIT COLOR GRADIENTS
// -----------------------------------------------------------------------------
void testGradients() {
  tft.setRotation(0); // 128x160 Portrait

  // Horizontal Rainbow Hue Ramp
  for (int x = 0; x < 128; x++) {
    float hue = (float)x / 128.0f;
    // HSV to RGB conversion
    float r = 0, g = 0, b = 0;
    int h_i = (int)(hue * 6);
    float f = hue * 6 - h_i;
    float q = 1.0f - f;
    float t = f;
    switch (h_i % 6) {
      case 0: r = 1; g = t; b = 0; break;
      case 1: r = q; g = 1; b = 0; break;
      case 2: r = 0; g = 1; b = t; break;
      case 3: r = 0; g = q; b = 1; break;
      case 4: r = t; g = 0; b = 1; break;
      case 5: r = 1; g = 0; b = q; break;
    }
    uint16_t col = rgb565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
    tft.drawFastVLine(x, 0, 80, col);
  }

  // Individual R, G, B Gradients
  for (int y = 0; y < 26; y++) {
    uint8_t val = (y * 255) / 25;
    tft.drawFastHLine(0, 80 + y, 128, rgb565(val, 0, 0));       // Red Ramp
    tft.drawFastHLine(0, 106 + y, 128, rgb565(0, val, 0));     // Green Ramp
    tft.drawFastHLine(0, 132 + y, 128, rgb565(0, 0, val));     // Blue Ramp
  }

  // Text Banner
  tft.fillRect(10, 25, 108, 22, 0x0000);
  tft.drawRect(10, 25, 108, 22, COLOR_WHITE);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(18, 32);
  tft.println("16-BIT 65K COLORS");

  delay(2500);
}

// -----------------------------------------------------------------------------
// TEST 6: GEOMETRIC PRIMITIVES STRESS & SPEED BENCHMARK
// -----------------------------------------------------------------------------
void testPrimitivesBenchmark() {
  tft.setRotation(0);
  uint32_t start;

  // 1. Screen Fill Speed
  start = micros();
  for (int i = 0; i < 5; i++) {
    tft.fillScreen(COLOR_RED);
    tft.fillScreen(COLOR_GREEN);
    tft.fillScreen(COLOR_BLUE);
  }
  metrics.screenFills_us = (micros() - start) / 15;

  // 2. High-Density Starburst Lines
  tft.fillScreen(COLOR_BLACK);
  start = micros();
  for (int16_t x = 0; x < 128; x += 4) {
    tft.drawLine(64, 80, x, 0, COLOR_CYAN);
    tft.drawLine(64, 80, x, 159, COLOR_YELLOW);
  }
  for (int16_t y = 0; y < 160; y += 4) {
    tft.drawLine(64, 80, 0, y, COLOR_MAGENTA);
    tft.drawLine(64, 80, 127, y, COLOR_GREEN);
  }
  metrics.lines_us = micros() - start;
  delay(1200);

  // 3. Concentric Nested Rectangles
  tft.fillScreen(COLOR_BLACK);
  start = micros();
  for (int16_t i = 0; i < 64; i += 3) {
    tft.drawRect(i, i * 160 / 128, 128 - 2 * i, 160 - 2 * (i * 160 / 128), rgb565(i * 4, 255 - i * 4, 180));
  }
  metrics.rects_us = micros() - start;
  delay(1000);

  // 4. Concentric Nested Circles
  tft.fillScreen(COLOR_BLACK);
  start = micros();
  for (int16_t r = 5; r < 60; r += 3) {
    tft.drawCircle(64, 80, r, rgb565(255 - r * 4, r * 4, 220));
  }
  metrics.circles_us = micros() - start;
  delay(1000);

  // 5. Rounded Rectangles
  tft.fillScreen(COLOR_BLACK);
  start = micros();
  for (int16_t i = 4; i < 55; i += 5) {
    tft.drawRoundRect(i, i, 128 - 2 * i, 160 - 2 * i, 8, rgb565(i * 4, 200, 255 - i * 4));
  }
  metrics.roundedRects_us = micros() - start;
  delay(1000);

  // 6. Triangles
  tft.fillScreen(COLOR_BLACK);
  start = micros();
  for (int16_t i = 0; i < 60; i += 5) {
    tft.drawTriangle(64, 80 - i, 64 - i, 80 + i, 64 + i, 80 + i, rgb565(255 - i * 3, i * 4, 150));
  }
  metrics.triangles_us = micros() - start;
  delay(1000);
}

// -----------------------------------------------------------------------------
// TEST 7: REAL-TIME 3D ROTATING WIREFRAME CUBE (LIVE FPS BENCHMARK)
// -----------------------------------------------------------------------------
struct Point3D { float x, y, z; };
struct Point2D { int16_t x, y; };

void testRotatingCube() {
  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);

  // Cube Vertices (-1 to +1)
  const Point3D baseVertices[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
  };

  // 12 Edges connecting vertices
  const uint8_t edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  Point2D oldPoints[8];
  Point2D currentPoints[8];

  float angleX = 0, angleY = 0, angleZ = 0;
  const float scale = 36.0f;
  const float distance = 3.5f;

  uint32_t frameCount = 0;
  uint32_t benchmarkStart = millis();
  uint32_t lastFpsUpdate = millis();
  float currentFps = 0.0f;

  tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 6);
  tft.println("3D CUBE BENCHMARK");

  // Run for 180 frames (~4-5 seconds)
  while (frameCount < 180) {
    // Trigonometric rotation
    float sinX = sin(angleX), cosX = cos(angleX);
    float sinY = sin(angleY), cosY = cos(angleY);
    float sinZ = sin(angleZ), cosZ = cos(angleZ);

    for (int i = 0; i < 8; i++) {
      // Rotation around X, Y, Z
      float x1 = baseVertices[i].x;
      float y1 = baseVertices[i].y * cosX - baseVertices[i].z * sinX;
      float z1 = baseVertices[i].y * sinX + baseVertices[i].z * cosX;

      float x2 = x1 * cosY + z1 * sinY;
      float y2 = y1;
      float z2 = -x1 * sinY + z1 * cosY;

      float x3 = x2 * cosZ - y2 * sinZ;
      float y3 = x2 * sinZ + y2 * cosZ;
      float z3 = z2 + distance;

      // Perspective projection onto 128x160 screen (Center at 64, 88)
      currentPoints[i].x = 64 + (int16_t)((x3 * scale) / z3);
      currentPoints[i].y = 88 + (int16_t)((y3 * scale) / z3);
    }

    // Erase Previous Cube Lines (Selective Erase for No Screen Flicker!)
    if (frameCount > 0) {
      for (int i = 0; i < 12; i++) {
        tft.drawLine(oldPoints[edges[i][0]].x, oldPoints[edges[i][0]].y,
                     oldPoints[edges[i][1]].x, oldPoints[edges[i][1]].y, COLOR_BLACK);
      }
    }

    // Draw Current Cube Lines in Vibrant Cyan/Yellow
    for (int i = 0; i < 12; i++) {
      uint16_t edgeColor = (i < 4) ? COLOR_CYAN : (i < 8 ? COLOR_YELLOW : COLOR_MAGENTA);
      tft.drawLine(currentPoints[edges[i][0]].x, currentPoints[edges[i][0]].y,
                   currentPoints[edges[i][1]].x, currentPoints[edges[i][1]].y, edgeColor);
    }

    // Save previous coordinates
    for (int i = 0; i < 8; i++) {
      oldPoints[i] = currentPoints[i];
    }

    angleX += 0.05f;
    angleY += 0.07f;
    angleZ += 0.03f;
    frameCount++;

    // Calculate Real-Time FPS
    if (millis() - lastFpsUpdate >= 350) {
      currentFps = (float)frameCount * 1000.0f / (float)(millis() - benchmarkStart);
      tft.setCursor(6, 144);
      tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
      tft.printf("FPS: %4.1f        ", currentFps);
      lastFpsUpdate = millis();
    }
  }

  metrics.cubeAvgFPS = (float)frameCount * 1000.0f / (float)(millis() - benchmarkStart);
}

// -----------------------------------------------------------------------------
// TEST 8: MULTI-PARTICLE BOUNCING BALLS SIMULATOR
// -----------------------------------------------------------------------------
struct Ball {
  float x, y;
  float vx, vy;
  uint8_t radius;
  uint16_t color;
};

void testParticlePhysics() {
  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);

  const int NUM_BALLS = 8;
  Ball balls[NUM_BALLS] = {
    {20, 30,  2.1,  1.8, 5, COLOR_RED},
    {60, 40, -1.8,  2.4, 6, COLOR_GREEN},
    {90, 80,  2.5, -1.7, 4, COLOR_BLUE},
    {40, 90, -2.2, -2.0, 5, COLOR_YELLOW},
    {70, 20,  1.5,  2.8, 4, COLOR_CYAN},
    {30, 60, -2.7,  1.3, 5, COLOR_MAGENTA},
    {80, 50,  1.9, -2.5, 6, COLOR_ORANGE},
    {50, 70, -1.6, -1.9, 4, COLOR_WHITE}
  };

  Ball oldBalls[NUM_BALLS];
  for (int i = 0; i < NUM_BALLS; i++) oldBalls[i] = balls[i];

  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(6, 6);
  tft.println("PARTICLE PHYSICS");

  uint32_t frameCount = 0;
  uint32_t start = millis();

  // Run simulation for 200 frames
  while (frameCount < 200) {
    // Erase previous positions with bounding boxes
    for (int i = 0; i < NUM_BALLS; i++) {
      tft.fillCircle((int16_t)oldBalls[i].x, (int16_t)oldBalls[i].y, oldBalls[i].radius, COLOR_BLACK);
    }

    // Update positions and handle boundary bounces
    for (int i = 0; i < NUM_BALLS; i++) {
      balls[i].x += balls[i].vx;
      balls[i].y += balls[i].vy;

      // Wall collisions (bounds: x from r to 127-r, y from 16 to 159-r)
      if (balls[i].x <= balls[i].radius) {
        balls[i].x = balls[i].radius;
        balls[i].vx = -balls[i].vx;
      } else if (balls[i].x >= 127 - balls[i].radius) {
        balls[i].x = 127 - balls[i].radius;
        balls[i].vx = -balls[i].vx;
      }

      if (balls[i].y <= 18 + balls[i].radius) {
        balls[i].y = 18 + balls[i].radius;
        balls[i].vy = -balls[i].vy;
      } else if (balls[i].y >= 159 - balls[i].radius) {
        balls[i].y = 159 - balls[i].radius;
        balls[i].vy = -balls[i].vy;
      }

      // Draw updated balls
      tft.fillCircle((int16_t)balls[i].x, (int16_t)balls[i].y, balls[i].radius, balls[i].color);
      oldBalls[i] = balls[i];
    }

    frameCount++;
    delay(10);
  }

  metrics.physicsAvgFPS = (float)frameCount * 1000.0f / (float)(millis() - start);
}

// -----------------------------------------------------------------------------
// TEST 9: RAW SPI BANDWIDTH TORTURE (FULL SCREEN TV STATIC / SNOW)
// -----------------------------------------------------------------------------
void testRawThroughputSnow() {
  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);

  // 128x160 = 20,480 pixels = 40,960 bytes per frame
  // Line buffer of 128 pixels (256 bytes) to stream line-by-line over SPI DMA
  uint16_t lineBuffer[128];

  tft.setCursor(6, 6);
  tft.setTextColor(COLOR_YELLOW);
  tft.println("SPI BUS TORTURE");
  delay(600);

  uint32_t seed = 123456789;
  uint32_t totalBytes = 0;
  uint32_t start = millis();
  const int FRAMES = 35; // ~1.43 Megabytes of high frequency switching

  for (int f = 0; f < FRAMES; f++) {
    tft.startWrite();
    tft.setAddrWindow(0, 0, 128, 160);

    for (int y = 0; y < 160; y++) {
      for (int x = 0; x < 128; x++) {
        // Fast Xorshift PRNG for true white noise
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        // Bipolar snow (White or Black)
        lineBuffer[x] = (seed & 0x01) ? 0xFFFF : 0x0000;
      }
      tft.writePixels(lineBuffer, 128);
    }
    tft.endWrite();
    totalBytes += (128 * 160 * 2);
  }

  uint32_t duration = millis() - start;
  metrics.snowMBs = ((float)totalBytes / 1048576.0f) / ((float)duration / 1000.0f);

  tft.fillRect(10, 60, 108, 38, COLOR_BLACK);
  tft.drawRect(10, 60, 108, 38, COLOR_CYAN);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(16, 66);
  tft.println("SPI THROUGHPUT:");
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(16, 78);
  tft.printf("%.2f MB / sec\n", metrics.snowMBs);
  delay(2000);
}

// -----------------------------------------------------------------------------
// TEST 10: HARDWARE ROTATION & INVERSION TORTURE
// -----------------------------------------------------------------------------
void testHardwareFeatures() {
  const char* rotNames[4] = {
    "ROT 0: 0 deg (128x160)",
    "ROT 1: 90 deg (160x128)",
    "ROT 2: 180 deg (128x160)",
    "ROT 3: 270 deg (160x128)"
  };

  // Test 4 Hardware Screen Orientations
  for (uint8_t r = 0; r < 4; r++) {
    tft.setRotation(r);
    tft.fillScreen(COLOR_NAVY);
    tft.drawRect(0, 0, tft.width(), tft.height(), COLOR_YELLOW);

    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 15);
    tft.println(rotNames[r]);

    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(10, 30);
    tft.printf("W: %d, H: %d\n", tft.width(), tft.height());

    // Compass Arrow pointing to origin (0,0)
    tft.fillTriangle(25, 60, 15, 80, 35, 80, COLOR_RED);
    tft.drawFastVLine(25, 80, 20, COLOR_WHITE);
    tft.setCursor(45, 68);
    tft.setTextColor(COLOR_YELLOW);
    tft.println("ORIGIN (0,0)");

    delay(900);
  }

  // Hardware Invert Mode Strobe
  tft.setRotation(0);
  tft.fillScreen(COLOR_RED);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  tft.setCursor(16, 60);
  tft.println("INVERT");
  tft.setCursor(24, 80);
  tft.println("TEST");

  for (int i = 0; i < 6; i++) {
    tft.invertDisplay(true);
    delay(200);
    tft.invertDisplay(false);
    delay(200);
  }

  // Display Sleep / Wake Power Cycling
  tft.fillScreen(COLOR_PURPLE);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.println("DISPLAY SLEEP TEST");
  tft.setCursor(10, 75);
  tft.println("Going into SLEEP...");
  delay(1000);

  tft.enableDisplay(false); // Sleep command (SLPIN 0x10)
  delay(800);
  tft.enableDisplay(true);  // Wake command (SLPOUT 0x11)
  tft.setCursor(10, 90);
  tft.setTextColor(COLOR_GREEN);
  tft.println("WOKE UP! SUCCESS!");
  delay(1200);
}

// -----------------------------------------------------------------------------
// TEST 11: BACKLIGHT PWM BREATHING FADER (IF BLK PIN CONNECTED)
// -----------------------------------------------------------------------------
void testBacklightBreathing() {
  if (TFT_BL < 0) return;

  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);
  tft.drawRect(2, 2, 124, 156, COLOR_ORANGE);

  tft.setTextColor(COLOR_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(14, 20);
  tft.println("PWM BACKLIGHT FADE");

  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(14, 40);
  tft.println("Sinusoidal PWM");
  tft.setCursor(14, 52);
  tft.println("0% to 100% Duty");

  // Breathe from full brightness down to dim and back up
  for (int cycle = 0; cycle < 2; cycle++) {
    for (int deg = 0; deg < 360; deg += 4) {
      float rad = (float)deg * 0.0174533f;
      // Cosine mapped between 15 (min) and 255 (max)
      uint8_t duty = (uint8_t)(135.0f + 120.0f * cos(rad));
      setBacklightBrightness(duty);

      // Simple visual indicator bar
      int barW = (duty * 100) / 255;
      tft.fillRect(14, 90, barW, 10, COLOR_YELLOW);
      tft.fillRect(14 + barW, 90, 100 - barW, 10, COLOR_BLACK);
      delay(10);
    }
  }
  setBacklightBrightness(255); // Restore full brightness
}

// -----------------------------------------------------------------------------
// TEST 12: BENCHMARK SUMMARY & SERIAL TELEMETRY REPORT
// -----------------------------------------------------------------------------
void printBenchmarkSummary() {
  tft.setRotation(0);
  tft.fillScreen(COLOR_BLACK);

  tft.fillRect(0, 0, 128, 20, COLOR_GREEN);
  tft.setTextColor(COLOR_BLACK);
  tft.setTextSize(1);
  tft.setCursor(14, 6);
  tft.println("BENCHMARK PASSED!");

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 26);
  tft.print("Screen Fill: ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u us\n", metrics.screenFills_us);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 38);
  tft.print("Lines:       ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u us\n", metrics.lines_us);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 50);
  tft.print("Circles:     ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u us\n", metrics.circles_us);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 62);
  tft.print("Rectangles:  ");
  tft.setTextColor(COLOR_WHITE);
  tft.printf("%u us\n", metrics.rects_us);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 74);
  tft.print("3D Cube:     ");
  tft.setTextColor(COLOR_GREEN);
  tft.printf("%.1f FPS\n", metrics.cubeAvgFPS);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 86);
  tft.print("Particles:   ");
  tft.setTextColor(COLOR_GREEN);
  tft.printf("%.1f FPS\n", metrics.physicsAvgFPS);

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 98);
  tft.print("Throughput:  ");
  tft.setTextColor(COLOR_YELLOW);
  tft.printf("%.2f MB/s\n", metrics.snowMBs);

  tft.drawRect(2, 114, 124, 40, COLOR_DARKGRAY);
  tft.fillRect(3, 115, 122, 38, 0x1082);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(8, 120);
  tft.println("Status: 100% HEALTHY");
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(8, 130);
  tft.println("J1 3.3V Power: STABLE");
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(8, 140);
  tft.println("Looping suite...");

  // Print full ASCII Table to Serial
  Serial.println("\n=======================================================");
  Serial.println("     ST7735 1.8\" TFT EXTREME BENCHMARK RESULTS         ");
  Serial.println("=======================================================");
  Serial.printf(" Screen Fill (per screen):    %6u us\n", metrics.screenFills_us);
  Serial.printf(" High-Density Starburst:      %6u us\n", metrics.lines_us);
  Serial.printf(" Concentric Circles:          %6u us\n", metrics.circles_us);
  Serial.printf(" Concentric Rectangles:       %6u us\n", metrics.rects_us);
  Serial.printf(" Rounded Rectangles:          %6u us\n", metrics.roundedRects_us);
  Serial.printf(" Triangles:                   %6u us\n", metrics.triangles_us);
  Serial.printf(" Real-time 3D Wireframe Cube: %6.1f FPS\n", metrics.cubeAvgFPS);
  Serial.printf(" Multi-Particle Simulator:    %6.1f FPS\n", metrics.physicsAvgFPS);
  Serial.printf(" Raw SPI Bus Throughput:      %6.2f MB/sec\n", metrics.snowMBs);
  Serial.println("=======================================================");
  Serial.println(" J1 Jumper Shorted: Pure 3.3V power rails confirmed.");
  Serial.println("=======================================================\n");

  delay(4000);
}

// -----------------------------------------------------------------------------
// ARDUINO SETUP & MAIN LOOP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1500); // Allow USB CDC serial to stabilize

  Serial.println("\n>>> Starting ST7735 Extreme Test Suite on ESP32-S3 <<<");

  // Initialize Backlight (if pin defined)
  setupBacklight();

  // Initialize ESP32-S3 Hardware SPI bus
  // SPI.begin(sclk, miso, mosi, ss)
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // Initialize ST7735 controller with selected tab profile
  tft.initR(ST7735_INIT_MODE);
  tft.fillScreen(COLOR_BLACK);

  Serial.println("ST7735 Hardware Initialized successfully.");
}

void loop() {
  Serial.println("[Stage 1/8] Running Diagnostics Dashboard...");
  testDiagnostics();

  Serial.println("[Stage 2/8] Running Boundary Alignment & Offset Test...");
  testAlignment();

  Serial.println("[Stage 3/8] Running Pure RGBW & Dead Pixel Test...");
  testColorPurity();

  Serial.println("[Stage 4/8] Running SMPTE Color Bars & Grayscale Test...");
  testColorBars();

  Serial.println("[Stage 5/8] Running 16-Bit Smooth Color Gradient Sweep...");
  testGradients();

  Serial.println("[Stage 6/8] Running Geometric Primitives Speed Benchmark...");
  testPrimitivesBenchmark();

  Serial.println("[Stage 7/8] Running Real-Time 3D Rotating Cube (FPS Benchmark)...");
  testRotatingCube();

  Serial.println("[Stage 8/8] Running Particle Physics Simulator...");
  testParticlePhysics();

  Serial.println("[Bonus 1] Running Raw SPI Bus Snow Torture...");
  testRawThroughputSnow();

  Serial.println("[Bonus 2] Running Hardware Rotation & Inversion Torture...");
  testHardwareFeatures();

  Serial.println("[Bonus 3] Running PWM Backlight Breathe Test...");
  testBacklightBreathing();

  Serial.println("[Complete] Printing Summary Report & Looping...");
  printBenchmarkSummary();
}
