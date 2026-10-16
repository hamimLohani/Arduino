# ESP32-S3 + INMP441 + ST7735: AI Speech-to-Text Transcriber

A real-time Speech-to-Text system running on an **ESP32-S3**, capturing speech from an **INMP441 digital I2S microphone**, streaming to **Whisper AI**, and displaying the transcribed text on an **ST7735 1.8" Color TFT LCD** (128x160).

---

## 1. Hardware Wiring

### ST7735 1.8" TFT Display (SPI)
| ST7735 Pin | Label on PCB | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **GND** | GND | **GND** | Common ground |
| **VCC** | VCC | **3.3V** | Power (J1 jumper shorted on display PCB) |
| **SCL / SCK** | SCL / CLK | **GPIO 12** | Hardware SPI Clock |
| **SDA / MOSI** | SDA / DIN | **GPIO 11** | Hardware SPI MOSI |
| **RES / RST** | RES / RST | **GPIO 9** | Hardware Reset |
| **DC / RS** | DC / A0 | **GPIO 8** | Data / Command Select |
| **CS** | CS | **GPIO 10** | SPI Chip Select |
| **BLK / LED** | BLK / LED | **GPIO 7** | Backlight control (or tie directly to 3.3V) |

### INMP441 Digital Microphone (I2S)
> ⚠️ **CAUTION:** INMP441 operates strictly on **3.3V**. Connecting to 5V will permanently damage the microphone!

| INMP441 Pin | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- |
| **VDD** | **3.3V** | Power supply |
| **GND** | **GND** | Common ground |
| **L/R** | **GND** | Tied to GND for Left audio channel |
| **WS** (Word Select) | **GPIO 4** | I2S Word Select / LRCLK |
| **SCK** (Serial Clock)| **GPIO 5** | I2S Bit Clock / BCLK |
| **SD** (Serial Data)  | **GPIO 6** | I2S Serial Data Out / DIN |

### Controls
| Control | Pin | Notes |
| :--- | :--- | :--- |
| **BOOT Button** | **GPIO 0** | ESP32-S3 onboard button. Hold down to talk, release to transcribe! |

---

## 2. Configuration (in `ESP32S3-Voice-Transcriber.ino`)

Open [`ESP32S3-Voice-Transcriber.ino`](file:///Users/Inz_mac/Developer/Arduino/ESP32S3-Voice-Transcriber/ESP32S3-Voice-Transcriber.ino) and edit:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Choose AI Provider:
// true  = Groq Whisper (RECOMMENDED: 100% Free tier, super fast ~300ms)
// false = OpenAI Whisper
#define USE_GROQ_WHISPER  true

const char* API_KEY = "YOUR_API_KEY_HERE";
```

> **Tip for Free API Key:** You can generate a free API key instantly at [Groq Console](https://console.groq.com/keys). Groq runs Whisper Large v3 Turbo in ~300 milliseconds at zero cost.

---

## 3. How to Compile and Upload

### Via Arduino CLI (Terminal)
```bash
# 1. Compile
/opt/homebrew/bin/arduino-cli compile -b esp32:esp32:esp32s3 /Users/Inz_mac/Developer/Arduino/ESP32S3-Voice-Transcriber

# 2. Upload (replace /dev/cu.usbmodem* with your port)
/opt/homebrew/bin/arduino-cli upload -b esp32:esp32:esp32s3 -p /dev/cu.usbmodem* /Users/Inz_mac/Developer/Arduino/ESP32S3-Voice-Transcriber
```

### Via Arduino IDE
1. Open [`ESP32S3-Voice-Transcriber.ino`](file:///Users/Inz_mac/Developer/Arduino/ESP32S3-Voice-Transcriber/ESP32S3-Voice-Transcriber.ino).
2. Select Board: **ESP32S3 Dev Module**.
3. Select your Serial Port.
4. Click **Upload**.

---

## 4. How It Works

1. **Instant Hardware Test Mode (No WiFi / No API Key needed initially)**:
   - If Wi-Fi or API key is not configured, the board immediately starts in **Live Hardware Test Mode**.
   - Speak into the INMP441 microphone: the screen shows a real-time **VU volume meter** and flips between `"TEST: LISTENING..."` and `"VOICE DETECTED!"`.
   - This verifies that your display and microphone wiring are 100% working before setting up network credentials.

2. **Full Speech-to-Text Mode**:
   - Once Wi-Fi connects, the screen displays `READY - HOLD BOOT`.
   - **Press & hold the onboard BOOT button** (GPIO 0) and speak naturally into the INMP441 microphone. The screen's live VU meter visualizes your voice levels.
   - **Release the button** when you finish speaking.
   - The ESP32 builds an uncompressed 16kHz 16-bit mono WAV in memory and streams it over HTTPS to Whisper AI.
   - Within seconds, your spoken words are word-wrapped and rendered clearly on the screen!
