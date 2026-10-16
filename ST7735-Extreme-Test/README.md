# ST7735 1.8" TFT (128x160) Extreme Hardware Torture & Benchmark Suite

This comprehensive benchmark and stress-testing firmware is designed specifically for **ESP32-S3** paired with an **ST7735 1.8-inch 128x160 SPI TFT LCD**.

---

## ⚡ Hardware Setup & J1 Jumper Note

> **J1 Jumper Status**: **SHORTED** ✅  
> When `J1` is shorted with solder on the back of the TFT module, the on-board 3.3V linear regulator (LDO) is bypassed. This allows you to feed **pure 3.3V from the ESP32-S3 directly into the `VCC` pin** without suffering from voltage drops (~2.8V) that cause display flickering, washed out colors, or SPI transfer corruption.

### Wiring Table (ESP32-S3 Hardware SPI)

| TFT Pin Label | Description | ESP32-S3 GPIO | Note |
|---|---|---|---|
| **GND** | Ground | **GND** | Common ground |
| **VCC** | Power Supply | **3.3V** | Direct 3.3V rail (J1 shorted) |
| **SCL / SCK / CLK** | SPI Clock | **GPIO 12** | ESP32-S3 Hardware SPI SCLK |
| **SDA / MOSI / DIN**| SPI Data In | **GPIO 11** | ESP32-S3 Hardware SPI MOSI |
| **RES / RST** | Reset | **GPIO 9** | Hardware reset |
| **DC / A0 / RS** | Data/Command | **GPIO 8** | Register select |
| **CS / SS** | Chip Select | **GPIO 10** | SPI Chip Select |
| **BLK / LED** | Backlight | **GPIO 7** *(or 3.3V)* | PWM Dimming support (or tie directly to 3.3V) |

*Note: If your BLK pin is tied directly to 3.3V, set `#define TFT_BL -1` in [`ST7735-Extreme-Test.ino`](file:///Users/Inz_mac/Developer/Arduino/ST7735-Extreme-Test/ST7735-Extreme-Test.ino).*

---

## 🧪 Included Torture & Stress Tests

1. **System & Telemetry Dashboard**:
   - Queries ESP32-S3 CPU frequency (240MHz dual-core), flash size, PSRAM status (OPI), and free heap memory.
   - Displays real-time hardware status card and animated boot bar.

2. **Pixel-Perfect Alignment & Offset Test**:
   - Outlines 1-pixel exact perimeter bounding box `(0, 0)` to `(127, 159)`.
   - Distinct 4-corner markers (Red, Green, Blue, Yellow) and center crosshairs.
   - Detects whether your ST7735 panel requires `INITR_BLACKTAB` (default), `INITR_18GREENTAB`, or `INITR_REDTAB`.

3. **Full RGBW & Dead Pixel Screen Flushes**:
   - Full-screen purities (Red, Green, Blue, White, Black, Yellow, Cyan, Magenta) to identify dead/stuck subpixels and backlight bleed.

4. **SMPTE Television Color Bars & 16-Step Grayscale**:
   - Standard 7-color broadcast bar pattern with a 16-step grayscale gradation ramp to test gamma curve accuracy and black-level clipping.

5. **Smooth 16-Bit Spectrum Gradients**:
   - Full 65,536-color rainbow hue ramp along with individual Red, Green, and Blue depth sweeps.

6. **Microsecond Geometric Primitives Benchmark**:
   - Measures draw execution times in microseconds ($\mu\text{s}$) for Screen Fills, Starburst Moire lines, Concentric Rectangles, Concentric Circles, Rounded Rectangles, and Triangles.

7. **Real-Time 3D Wireframe Spinning Cube**:
   - Full 3D trigonometry rotation matrix with perspective projection.
   - Uses flicker-free selective line erasing.
   - Calculates and displays live real-time FPS directly on the display.

8. **Multi-Particle Physics Simulation**:
   - 8 independent velocity-driven spheres with elastic boundary collision physics.

9. **Raw SPI Bus Throughput Torture (Snow / TV Static)**:
   - High-frequency white noise generator streaming full 40KB frames over SPI to stress peak power draw and calculate sustained bus throughput in **MB/s**.

10. **Hardware Features & Inversion Torture**:
    - Tests 4 hardware screen orientations (0°, 90°, 180°, 270°).
    - Rapid hardware color inversion strobe (`invertDisplay`).
    - Power cycling display sleep and wake states (`enableDisplay`).

11. **PWM Backlight Breathing Fader**:
    - Smooth sinusoidal PWM dimming between 5% and 100% duty cycle (via ESP32-S3 LEDC).

12. **Continuous Looping & Telemetry Output**:
    - Comprehensive benchmark telemetry table output to Serial at **115200 baud**.
    - Continuous auto-cycling for burn-in and endurance testing.

---

## 🚀 How to Flash

Using Arduino CLI:
```bash
/opt/homebrew/bin/arduino-cli compile -b esp32:esp32:esp32s3 /Users/Inz_mac/Developer/Arduino/ST7735-Extreme-Test
/opt/homebrew/bin/arduino-cli upload -b esp32:esp32:esp32s3 -p /dev/cu.usbmodem* /Users/Inz_mac/Developer/Arduino/ST7735-Extreme-Test
```
