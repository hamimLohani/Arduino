/*
 * =====================================================================================
 *        ESP32-S3 + INMP441 + ST7735: AI SPEECH-TO-TEXT TRANSCRIBER
 * =====================================================================================
 *  Hardware Components:
 *   1. ESP32-S3 (DevKitC-1 or compatible)
 *   2. ST7735 1.8" TFT LCD Display (128x160 SPI)
 *   3. INMP441 I2S Omnidirectional MEMS Digital Microphone
 *   4. Built-in BOOT button (GPIO 0) for Push-to-Talk
 *
 *  Features:
 *   - Live Voice Activity & VU Meter (visual feedback while you talk)
 *   - Push-to-Talk via BOOT button (GPIO 0) + Auto-stop on release or max time
 *   - Generates 16kHz 16-bit Mono uncompressed WAV in-memory
 *   - Fast AI Speech Transcription via Groq Whisper (Free & ~300ms) or OpenAI Whisper
 *   - Word-wrapped text rendering on 160x128 color LCD screen
 *   - Fallback Live Mic Test mode if Wi-Fi / API Key is not yet configured
 *
 *  PIN CONFIGURATION:
 *  -------------------------------------------------------------------------------------
 *  ST7735 TFT Pin      ESP32-S3 GPIO     Notes
 *  -------------------------------------------------------------------------------------
 *  GND                 GND               Common Ground
 *  VCC                 3.3V              Power (J1 shorted for 3.3V direct)
 *  SCL / SCK           GPIO 12           SPI Hardware Clock
 *  SDA / MOSI          GPIO 11           SPI Hardware MOSI
 *  RES / RST           GPIO 9            Reset
 *  DC / A0             GPIO 8            Data / Command
 *  CS                  GPIO 10           Chip Select
 *  BLK / LED           GPIO 7 (or 3.3V)  Backlight
 *  -------------------------------------------------------------------------------------
 *  INMP441 Mic Pin     ESP32-S3 GPIO     Notes
 *  -------------------------------------------------------------------------------------
 *  VDD                 3.3V              Power (Do NOT connect to 5V!)
 *  GND                 GND               Ground
 *  L/R                 GND               Left channel
 *  WS                  GPIO 4            I2S Word Select (LRCLK)
 *  SCK                 GPIO 5            I2S Serial Clock (BCLK)
 *  SD                  GPIO 6            I2S Serial Data Out
 *  -------------------------------------------------------------------------------------
 *  Control             ESP32-S3 GPIO
 *  -------------------------------------------------------------------------------------
 *  BOOT Button         GPIO 0            Push & Hold to Talk (Release to Transcribe)
 * =====================================================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <driver/i2s.h>

// =============================================================================
// 1. USER CONFIGURATION (WIFI & AI API)
// =============================================================================
const char* WIFI_SSID     = "Tp Link 2.4 GHz";
const char* WIFI_PASSWORD = "flat_6b@";

// Select your AI Transcription Provider:
//  - Groq Whisper (RECOMMENDED): Free tier, ultra-fast (~300ms response time).
//    Get your free key at: https://console.groq.com/keys
//  - OpenAI Whisper: Standard OpenAI Whisper API.
//    Get your key at: https://platform.openai.com/api-keys
#define USE_GROQ_WHISPER  true   // Set to 'false' if using OpenAI directly

// Enter your API Key (Groq key starts with 'gsk_', OpenAI key starts with 'sk-'):
const char* API_KEY = "";

// =============================================================================
// 2. PIN DEFINITIONS
// =============================================================================
// ST7735 Display Pins
#define TFT_CS    10
#define TFT_DC     8
#define TFT_RST    9
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     7   // Set to -1 if backlight is connected directly to 3.3V

// INMP441 I2S Microphone Pins
#define I2S_WS_PIN    4   // Word Select (LRCK)
#define I2S_SCK_PIN   5   // Bit Clock (BCLK)
#define I2S_SD_PIN    6   // Serial Data (DIN)
#define I2S_PORT      I2S_NUM_0

// Push-to-Talk Button (ESP32-S3 onboard BOOT button)
#define BUTTON_PIN    0   // Active LOW

// =============================================================================
// 3. AUDIO & RECORDING SETTINGS
// =============================================================================
#define SAMPLE_RATE       16000    // 16 kHz (optimal for Whisper AI)
#define MAX_RECORD_SEC    5        // Maximum recording length in seconds
#define MIC_GAIN_BOOST    6        // Digital gain multiplier for INMP441 MEMS mic
#define SILENCE_THRESHOLD 350      // RMS threshold to detect talking vs silence

// Total audio buffer size for 16-bit mono PCM:
const size_t MAX_AUDIO_BYTES = (size_t)MAX_RECORD_SEC * SAMPLE_RATE * sizeof(int16_t);
const size_t WAV_HEADER_SIZE = 44;
const size_t TOTAL_BUFFER_SIZE = MAX_AUDIO_BYTES + WAV_HEADER_SIZE;

// Dynamic audio buffer pointer
uint8_t* wavBuffer = nullptr;
size_t recordedAudioBytes = 0;

// Hardware display object
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Modern Color Palette (RGB565)
#define COLOR_BG        0x0842   // Deep space navy
#define COLOR_CARD      0x10A4   // Dark slate panel
#define COLOR_HEADER    0x18E7   // Header bar
#define COLOR_WHITE     0xFFFF   // White text
#define COLOR_TEXT_MUT  0x94B2   // Muted grey text
#define COLOR_CYAN      0x067F   // Electric cyan
#define COLOR_GREEN     0x07E0   // Emerald green
#define COLOR_RED       0xF800   // Warning red
#define COLOR_ORANGE    0xFC00   // Active recording orange/amber
#define COLOR_YELLOW    0xFFE0   // Process yellow
#define COLOR_BORDER    0x2965   // Subtle boundary

// System States
enum SystemState {
  STATE_INIT,
  STATE_IDLE,
  STATE_RECORDING,
  STATE_TRANSCRIBING,
  STATE_SHOW_RESULT,
  STATE_HARDWARE_TEST_MODE
};

SystemState currentState = STATE_INIT;
bool isConfigured = false;
String lastTranscription = "Press and hold BOOT button to speak!";

// Forward declarations
void initDisplay();
bool initI2SMicrophone();
void createWavHeader(uint8_t* header, int totalDataLen, int sampleRate, int channels, int bitsPerSample);
int16_t readSampleAndRMS(int32_t& rawSample);
String sendAudioToWhisper(uint8_t* wavData, size_t wavSize);
void drawUIHeader(const char* title, bool wifiOk);
void drawStatusBadge(const char* label, uint16_t badgeColor);
void drawVUMeter(int rmsLevel);
void drawTranscript(const String& text, uint16_t textColor = COLOR_WHITE);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n==========================================");
  Serial.println("  ESP32-S3 AI Speech-to-Text Transcriber  ");
  Serial.println("==========================================");

  // Initialize BOOT button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize Display
  initDisplay();
  drawUIHeader("AI VOICE STT", false);
  drawStatusBadge("INITIALIZING...", COLOR_CYAN);

  // Allocate audio memory (prefer PSRAM if available)
  if (psramFound()) {
    wavBuffer = (uint8_t*)ps_malloc(TOTAL_BUFFER_SIZE);
    Serial.printf("[RAM] Allocated %u bytes in PSRAM\n", TOTAL_BUFFER_SIZE);
  } else {
    wavBuffer = (uint8_t*)malloc(TOTAL_BUFFER_SIZE);
    Serial.printf("[RAM] Allocated %u bytes in Internal Heap\n", TOTAL_BUFFER_SIZE);
  }

  if (wavBuffer == nullptr) {
    Serial.println("[ERR] Failed to allocate audio memory!");
    drawStatusBadge("RAM ALLOC FAIL", COLOR_RED);
    while (true) delay(1000);
  }

  // Initialize INMP441 Microphone via I2S
  if (!initI2SMicrophone()) {
    Serial.println("[ERR] I2S Microphone initialization failed!");
    drawStatusBadge("I2S MIC FAIL", COLOR_RED);
    while (true) delay(1000);
  }
  Serial.println("[I2S] INMP441 Microphone Initialized successfully.");

  // Check if user has entered real Wi-Fi credentials
  if (String(WIFI_SSID) != "YOUR_WIFI_SSID" && String(API_KEY) != "YOUR_API_KEY_HERE") {
    isConfigured = true;
    drawStatusBadge("CONNECTING WIFI", COLOR_YELLOW);
    Serial.printf("[WIFI] Connecting to %s...\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 25) {
      delay(300);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());
      drawUIHeader("AI VOICE STT", true);
      drawStatusBadge("READY - HOLD BOOT", COLOR_GREEN);
      drawTranscript("Hold BOOT button, talk into microphone, then release.");
      currentState = STATE_IDLE;
    } else {
      Serial.println("\n[WIFI] Connection failed. Entering Hardware Test Mode.");
      drawUIHeader("AI VOICE STT", false);
      drawStatusBadge("WIFI FAILED", COLOR_RED);
      drawTranscript("WiFi failed. Running in Live Mic Meter Test Mode.");
      delay(2000);
      currentState = STATE_HARDWARE_TEST_MODE;
    }
  } else {
    isConfigured = false;
    Serial.println("[NOTE] WiFi or API Key not configured yet.");
    Serial.println("[NOTE] Running in LIVE HARDWARE TEST MODE (Mic + Display).");
    drawUIHeader("MIC TEST MODE", false);
    drawStatusBadge("TEST: TALK TO MIC", COLOR_CYAN);
    drawTranscript("Hardware Test Mode:\nTalk into INMP441 to test VU meter & sound detection.\n\nSet WiFi/API Key in code for AI transcription.");
    delay(2500);
    currentState = STATE_HARDWARE_TEST_MODE;
  }
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
  // ---------------------------------------------------------------------------
  // MODE 1: HARDWARE TEST MODE (Live Voice Activity & VU Meter without Wi-Fi)
  // ---------------------------------------------------------------------------
  if (currentState == STATE_HARDWARE_TEST_MODE) {
    int32_t i2sSamples[64];
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, i2sSamples, sizeof(i2sSamples), &bytesRead, portMAX_DELAY);

    int samplesCount = bytesRead / sizeof(int32_t);
    int64_t sumSquares = 0;

    for (int i = 0; i < samplesCount; i++) {
      // INMP441 is 24-bit left aligned in 32-bit slot
      int16_t pcm16 = (int16_t)((i2sSamples[i] >> 14) * MIC_GAIN_BOOST);
      sumSquares += (int64_t)pcm16 * pcm16;
    }

    int rms = (samplesCount > 0) ? (int)sqrt(sumSquares / samplesCount) : 0;
    drawVUMeter(rms);

    static unsigned long lastLabelUpdate = 0;
    static bool wasTalking = false;
    bool isTalking = (rms > SILENCE_THRESHOLD);

    if (millis() - lastLabelUpdate > 150) {
      if (isTalking != wasTalking) {
        if (isTalking) {
          drawStatusBadge("VOICE DETECTED!", COLOR_GREEN);
        } else {
          drawStatusBadge("TEST: LISTENING...", COLOR_CYAN);
        }
        wasTalking = isTalking;
      }
      lastLabelUpdate = millis();
    }
    delay(10);
    return;
  }

  // ---------------------------------------------------------------------------
  // MODE 2: AI SPEECH-TO-TEXT WORKFLOW
  // ---------------------------------------------------------------------------
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

  switch (currentState) {
    case STATE_IDLE: {
      // Idle live VU meter preview (lets you see room audio levels)
      int32_t i2sSamples[32];
      size_t bytesRead = 0;
      i2s_read(I2S_PORT, i2sSamples, sizeof(i2sSamples), &bytesRead, 10);
      int samplesCount = bytesRead / sizeof(int32_t);
      int64_t sumSq = 0;
      for (int i = 0; i < samplesCount; i++) {
        int16_t pcm = (int16_t)((i2sSamples[i] >> 14) * MIC_GAIN_BOOST);
        sumSq += (int64_t)pcm * pcm;
      }
      int rms = (samplesCount > 0) ? (int)sqrt(sumSq / samplesCount) : 0;
      drawVUMeter(rms);

      // Trigger recording on BOOT button press
      if (buttonPressed) {
        Serial.println("[REC] Button pressed! Starting recording...");
        drawStatusBadge("RECORDING... TALK!", COLOR_ORANGE);
        drawTranscript("Recording speech...\nSpeak clearly into the microphone.");

        // Clear DMA buffer for crisp start
        i2s_zero_dma_buffer(I2S_PORT);
        recordedAudioBytes = 0;
        currentState = STATE_RECORDING;
      }
      delay(20);
      break;
    }

    case STATE_RECORDING: {
      // Read audio chunk from I2S
      int32_t rawChunk[128];
      size_t bytesRead = 0;
      i2s_read(I2S_PORT, rawChunk, sizeof(rawChunk), &bytesRead, portMAX_DELAY);

      int samplesRead = bytesRead / sizeof(int32_t);
      int16_t* pcmDest = (int16_t*)(wavBuffer + WAV_HEADER_SIZE + recordedAudioBytes);

      int64_t sumSq = 0;
      for (int i = 0; i < samplesRead; i++) {
        int32_t amplified = (rawChunk[i] >> 14) * MIC_GAIN_BOOST;
        // Clip protection
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;
        int16_t sample16 = (int16_t)amplified;

        pcmDest[i] = sample16;
        sumSq += (int64_t)sample16 * sample16;
      }

      recordedAudioBytes += (samplesRead * sizeof(int16_t));

      // Update live VU meter on screen
      int rms = (samplesRead > 0) ? (int)sqrt(sumSq / samplesRead) : 0;
      drawVUMeter(rms);

      // Stop recording if button released or maximum duration reached
      bool timeLimitReached = (recordedAudioBytes >= MAX_AUDIO_BYTES);
      if (!buttonPressed || timeLimitReached) {
        Serial.printf("[REC] Stopped. Recorded %u bytes (%.2f seconds).\n",
                      recordedAudioBytes, (float)recordedAudioBytes / (SAMPLE_RATE * 2));

        if (recordedAudioBytes < (SAMPLE_RATE * sizeof(int16_t) / 2)) {
          // Recording was less than 0.5s (accidental tap)
          drawStatusBadge("TOO SHORT - RETRY", COLOR_RED);
          drawTranscript("Hold button longer while speaking.");
          delay(1200);
          drawStatusBadge("READY - HOLD BOOT", COLOR_GREEN);
          currentState = STATE_IDLE;
        } else {
          // Create valid 44-byte WAV header at start of buffer
          createWavHeader(wavBuffer, recordedAudioBytes, SAMPLE_RATE, 1, 16);
          currentState = STATE_TRANSCRIBING;
        }
      }
      break;
    }

    case STATE_TRANSCRIBING: {
      drawStatusBadge("AI TRANSCRIBING...", COLOR_YELLOW);
      drawTranscript("Sending audio to Whisper AI model...");
      drawVUMeter(0);

      size_t totalWavSize = WAV_HEADER_SIZE + recordedAudioBytes;
      String transcription = sendAudioToWhisper(wavBuffer, totalWavSize);

      if (transcription.length() > 0) {
        lastTranscription = transcription;
        drawStatusBadge("TRANSCRIBED!", COLOR_GREEN);
        drawTranscript(lastTranscription, COLOR_CYAN);
      } else {
        drawStatusBadge("TRANSCRIPTION ERR", COLOR_RED);
        drawTranscript("Could not transcribe speech. Check API key, WiFi, or speak closer.");
      }

      currentState = STATE_SHOW_RESULT;
      break;
    }

    case STATE_SHOW_RESULT: {
      // Wait for user to press button again to make a new transcription
      if (buttonPressed) {
        delay(150); // Debounce
        currentState = STATE_IDLE;
      }
      delay(30);
      break;
    }

    default:
      currentState = STATE_IDLE;
      break;
  }
}

// =============================================================================
// 4. DISPLAY GRAPHICS & UI FUNCTIONS
// =============================================================================
void initDisplay() {
  // Backlight setup
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // Full brightness
  }

  // Initialize ST7735 in Hardware SPI mode
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); // Landscape: 160 x 128
  tft.fillScreen(COLOR_BG);
}

void drawUIHeader(const char* title, bool wifiOk) {
  // Top bar container
  tft.fillRect(0, 0, 160, 18, COLOR_HEADER);
  tft.drawFastHLine(0, 18, 160, COLOR_BORDER);

  // Title text
  tft.setTextColor(COLOR_WHITE, COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(6, 5);
  tft.print(title);

  // Wi-Fi Status Tag
  tft.setCursor(110, 5);
  if (wifiOk) {
    tft.setTextColor(COLOR_GREEN, COLOR_HEADER);
    tft.print("WiFi OK");
  } else {
    tft.setTextColor(COLOR_RED, COLOR_HEADER);
    tft.print("No WiFi");
  }
}

void drawStatusBadge(const char* label, uint16_t badgeColor) {
  tft.fillRect(4, 22, 152, 16, COLOR_CARD);
  tft.drawRect(4, 22, 152, 16, badgeColor);

  tft.setTextColor(badgeColor, COLOR_CARD);
  tft.setTextSize(1);

  // Center text in badge
  int len = strlen(label);
  int x = (160 - (len * 6)) / 2;
  if (x < 6) x = 6;
  tft.setCursor(x, 26);
  tft.print(label);
}

void drawVUMeter(int rmsLevel) {
  const int meterX = 6;
  const int meterY = 41;
  const int meterW = 148;
  const int meterH = 7;

  // Map RMS to meter width (logarithmic feel)
  int fillW = map(rmsLevel, 50, 4000, 0, meterW);
  if (fillW < 0) fillW = 0;
  if (fillW > meterW) fillW = meterW;

  // Background frame
  tft.drawRect(meterX - 1, meterY - 1, meterW + 2, meterH + 2, COLOR_BORDER);

  // Dynamic color based on audio volume
  uint16_t barColor = COLOR_CYAN;
  if (fillW > (meterW * 0.75)) {
    barColor = COLOR_RED;
  } else if (fillW > (meterW * 0.40)) {
    barColor = COLOR_YELLOW;
  } else if (fillW > 0) {
    barColor = COLOR_GREEN;
  }

  // Active bar
  if (fillW > 0) {
    tft.fillRect(meterX, meterY, fillW, meterH, barColor);
  }
  // Clear remaining
  if (meterW - fillW > 0) {
    tft.fillRect(meterX + fillW, meterY, meterW - fillW, meterH, COLOR_BG);
  }
}

void drawTranscript(const String& text, uint16_t textColor) {
  const int cardX = 4;
  const int cardY = 51;
  const int cardW = 152;
  const int cardH = 73;

  // Card background
  tft.fillRect(cardX, cardY, cardW, cardH, COLOR_CARD);
  tft.drawRect(cardX, cardY, cardW, cardH, COLOR_BORDER);

  tft.setTextColor(textColor, COLOR_CARD);
  tft.setTextSize(1);

  // Word-wrap printing logic (fits ~24 chars per line)
  int cursorX = cardX + 4;
  int cursorY = cardY + 5;
  const int maxCursorX = cardX + cardW - 6;
  const int maxCursorY = cardY + cardH - 10;

  int startIdx = 0;
  while (startIdx < text.length() && cursorY <= maxCursorY) {
    int nextSpace = text.indexOf(' ', startIdx);
    int nextNewline = text.indexOf('\n', startIdx);

    int endIdx;
    bool isNewline = false;

    if (nextNewline != -1 && (nextNewline < nextSpace || nextSpace == -1)) {
      endIdx = nextNewline;
      isNewline = true;
    } else if (nextSpace != -1) {
      endIdx = nextSpace;
    } else {
      endIdx = text.length();
    }

    String word = text.substring(startIdx, endIdx);
    int wordPixelWidth = word.length() * 6;

    // Wrap line if word exceeds width
    if (cursorX + wordPixelWidth > maxCursorX && cursorX > cardX + 4) {
      cursorX = cardX + 4;
      cursorY += 10;
      if (cursorY > maxCursorY) break;
    }

    tft.setCursor(cursorX, cursorY);
    tft.print(word);
    cursorX += wordPixelWidth;

    if (isNewline) {
      cursorX = cardX + 4;
      cursorY += 10;
      startIdx = endIdx + 1;
    } else {
      // Print space
      tft.setCursor(cursorX, cursorY);
      tft.print(' ');
      cursorX += 6;
      startIdx = endIdx + 1;
    }
  }
}

// =============================================================================
// 5. I2S MICROPHONE DRIVER & WAV FORMATTER
// =============================================================================
bool initI2SMicrophone() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) return false;

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) return false;

  i2s_zero_dma_buffer(I2S_PORT);
  return true;
}

void createWavHeader(uint8_t* header, int totalDataLen, int sampleRate, int channels, int bitsPerSample) {
  int totalFileLen = totalDataLen + 36;
  int byteRate = sampleRate * channels * (bitsPerSample / 8);
  int blockAlign = channels * (bitsPerSample / 8);

  // RIFF marker
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  header[4] = (uint8_t)(totalFileLen & 0xff);
  header[5] = (uint8_t)((totalFileLen >> 8) & 0xff);
  header[6] = (uint8_t)((totalFileLen >> 16) & 0xff);
  header[7] = (uint8_t)((totalFileLen >> 24) & 0xff);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

  // "fmt " sub-chunk
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16;  // Subchunk1Size = 16 for PCM
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;
  header[20] = 1;   // AudioFormat: 1 = uncompressed PCM
  header[21] = 0;
  header[22] = (uint8_t)channels;
  header[23] = 0;
  header[24] = (uint8_t)(sampleRate & 0xff);
  header[25] = (uint8_t)((sampleRate >> 8) & 0xff);
  header[26] = (uint8_t)((sampleRate >> 16) & 0xff);
  header[27] = (uint8_t)((sampleRate >> 24) & 0xff);
  header[28] = (uint8_t)(byteRate & 0xff);
  header[29] = (uint8_t)((byteRate >> 8) & 0xff);
  header[30] = (uint8_t)((byteRate >> 16) & 0xff);
  header[31] = (uint8_t)((byteRate >> 24) & 0xff);
  header[32] = (uint8_t)blockAlign;
  header[33] = 0;
  header[34] = (uint8_t)bitsPerSample;
  header[35] = 0;

  // "data" sub-chunk
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (uint8_t)(totalDataLen & 0xff);
  header[41] = (uint8_t)((totalDataLen >> 8) & 0xff);
  header[42] = (uint8_t)((totalDataLen >> 16) & 0xff);
  header[43] = (uint8_t)((totalDataLen >> 24) & 0xff);
}

// =============================================================================
// 6. WHISPER AI HTTPS TRANSCRIPTION CLIENT
// =============================================================================
String sendAudioToWhisper(uint8_t* wavData, size_t wavSize) {
  WiFiClientSecure client;
  client.setInsecure(); // Skip SSL cert check for speed & reliability on ESP32

  const char* host = USE_GROQ_WHISPER ? "api.groq.com" : "api.openai.com";
  const char* path = USE_GROQ_WHISPER ? "/openai/v1/audio/transcriptions" : "/v1/audio/transcriptions";
  const char* modelName = USE_GROQ_WHISPER ? "whisper-large-v3-turbo" : "whisper-1";

  Serial.printf("[HTTP] Connecting to %s:443...\n", host);
  if (!client.connect(host, 443)) {
    Serial.println("[HTTP] Connection to AI server failed!");
    return "Error: SSL Connection Failed";
  }

  // Construct Multipart Boundary & Payload Parts
  String boundary = "----ESP32S3BoundaryAiVoice789";

  String headPart = "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"model\"\r\n\r\n" +
                    modelName + "\r\n" +
                    "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"file\"; filename=\"speech.wav\"\r\n" +
                    "Content-Type: audio/wav\r\n\r\n";

  String tailPart = "\r\n--" + boundary + "--\r\n";

  size_t totalPayloadLength = headPart.length() + wavSize + tailPart.length();

  // Send HTTP POST headers
  client.printf("POST %s HTTP/1.1\r\n", path);
  client.printf("Host: %s\r\n", host);
  client.printf("Authorization: Bearer %s\r\n", API_KEY);
  client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", boundary.c_str());
  client.printf("Content-Length: %u\r\n", totalPayloadLength);
  client.print("Connection: close\r\n\r\n");

  // Send Multipart Header
  client.print(headPart);

  // Send WAV Audio in chunks (1024 bytes each to avoid saturating TCP buffers)
  size_t bytesRemaining = wavSize;
  size_t offset = 0;
  while (bytesRemaining > 0) {
    size_t chunk = (bytesRemaining > 1024) ? 1024 : bytesRemaining;
    client.write(wavData + offset, chunk);
    offset += chunk;
    bytesRemaining -= chunk;
  }

  // Send Multipart Tail
  client.print(tailPart);
  client.flush();

  Serial.println("[HTTP] Audio streamed. Awaiting Whisper AI response...");

  // Wait for response
  unsigned long startWait = millis();
  while (client.available() == 0) {
    if (millis() - startWait > 12000) {
      Serial.println("[HTTP] Response timeout!");
      client.stop();
      return "Error: Request Timed Out";
    }
    delay(20);
  }

  // Read response headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break; // Blank line marks end of headers
  }

  // Read response body
  String responseJson = "";
  while (client.available()) {
    responseJson += (char)client.read();
  }
  client.stop();

  Serial.println("[HTTP] Raw AI Response:\n" + responseJson);

  // Extract clean JSON bounds { ... }
  int jsonStart = responseJson.indexOf('{');
  int jsonEnd = responseJson.lastIndexOf('}');
  if (jsonStart == -1 || jsonEnd == -1) {
    return "Error: Invalid JSON response";
  }

  String cleanJson = responseJson.substring(jsonStart, jsonEnd + 1);

  // Parse JSON using ArduinoJson v7
  JsonDocument doc;
  DeserializationError jsonErr = deserializeJson(doc, cleanJson);
  if (jsonErr) {
    Serial.println("[JSON] Parse failed: " + String(jsonErr.c_str()));
    return "Error: JSON Parse Failed";
  }

  if (doc.containsKey("text")) {
    String transcribed = doc["text"].as<String>();
    transcribed.trim();
    Serial.println("[RESULT] Speech: " + transcribed);
    return transcribed;
  } else if (doc.containsKey("error")) {
    String errMsg = doc["error"]["message"].as<String>();
    Serial.println("[API ERR] " + errMsg);
    return "API Err: " + errMsg;
  }

  return "Error: No text found in response";
}
