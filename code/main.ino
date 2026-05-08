/*
* ══════════════════════════════════════════════
 * Edge AI SMPS Predictive Failure Detection
 * Author  : Jay Poriya (jayporiya03)
 * Model   : 95% Accuracy | 4 Classes
 * Hardware: ESP32 WROOM-32
 * Version : 3.0 FINAL CORRECTED
 * ══════════════════════════════════════════════
 *
 * ARDUINO IDE SETTINGS:
 * Board            → ESP32 Dev Module
 * Partition Scheme → Huge APP (3MB No OTA)
 * Upload Speed     → 115200
 *
 * GPIO MAP:
 * GPIO34 → ACS712 OUT (Current)
 * GPIO35 → Voltage Sensor OUT (0-25V)
 * GPIO4  → DS18B20 DATA (+ 4.7kΩ pullup)
 * GPIO21 → OLED SDA
 * GPIO22 → OLED SCL
 * GPIO26 → Relay IN (Active LOW)
 * GPIO27 → Buzzer +
 * GPIO25 → Red LED (via 220Ω)
 * ══════════════════════════════════════════════
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <jayporiya03-project-1_inferencing.h>

// ─── OLED CONFIG ─────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── GPIO PINS ───────────────────────────────
#define CURRENT_PIN   34
#define VOLTAGE_PIN   35
#define ONE_WIRE_BUS   4
#define RELAY_PIN     26
#define BUZZER_PIN    27
#define LED_RED_PIN   25

// ─── SENSOR OBJECTS ──────────────────────────
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ─── FEATURE BUFFER ──────────────────────────
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// ─── GLOBAL STATE ────────────────────────────
float gVoltage  = 0;
float gCurrent  = 0;
float gTemp     = 0;
float gPower    = 0;
float gConf     = 0;
String gClass   = "Normal";
bool isFault    = false;

// ─── BLINK CONTROL ───────────────────────────
bool blinkState       = false;
unsigned long lastBlink = 0;

// ══════════════════════════════════════════════
// ✅ FIX 1: VOLTAGE CALIBRATION
// Change 1.73 up/down until OLED matches multimeter
// ══════════════════════════════════════════════
float voltageCalibration = 1.73;

// ══════════════════════════════════════════════
// SENSOR FUNCTIONS
// ══════════════════════════════════════════════

float readVoltage() {
  // ✅ FIX 1: Was adc * 11.0 (WRONG) → Now calibrated
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(VOLTAGE_PIN);
    delay(1);
  }
  float raw = sum / 10.0;
  float adc = (raw / 4095.0) * 3.3;
  float voltage = adc * voltageCalibration;
  if (voltage < 0) voltage = 0;
  if (voltage > 25) voltage = 25;
  return voltage;
}

float readCurrent() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(CURRENT_PIN);
    delay(1);
  }
  float raw = sum / 10.0;
  float adc = (raw / 4095.0) * 3.3;
  // ACS712 5A  → 0.185
  // ACS712 20A → 0.100
  // ACS712 30A → 0.066
  float val = (adc - 2.5) / 0.185;
  if (abs(val) < 0.05) val = 0;
  return abs(val);
}

float readTemperature() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C || t == -127.0) {
    Serial.println("⚠️ DS18B20 disconnected!");
    return 25.0;
  }
  return t;
}

// ══════════════════════════════════════════════
// ✅ FIX 2: RELAY LOGIC CORRECTED
// Active LOW relay:
// Normal → LOW  = relay ON  (load connected)
// Fault  → HIGH = relay OFF (load disconnected)
// ══════════════════════════════════════════════

void handleOutputs(String cls) {
  if (cls == "Normal") {
    digitalWrite(RELAY_PIN,   LOW);   // ✅ Active LOW → ON
    digitalWrite(BUZZER_PIN,  LOW);
    digitalWrite(LED_RED_PIN, LOW);
    isFault = false;
  } else {
    digitalWrite(RELAY_PIN,   HIGH);  // ✅ Active LOW → OFF
    digitalWrite(LED_RED_PIN, HIGH);
    // Double beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    isFault = true;
  }
}

// ══════════════════════════════════════════════
// DRAW BAR
// ══════════════════════════════════════════════

void drawBar(int x, int y, int width, int height,
             float value, float maxVal) {
  display.drawRect(x, y, width, height, SSD1306_WHITE);
  int fillW = (int)((value / maxVal) * (width - 2));
  fillW = constrain(fillW, 0, width - 2);
  display.fillRect(x + 1, y + 1, fillW, height - 2, SSD1306_WHITE);
}

// ══════════════════════════════════════════════
// SCREEN 1 — NORMAL DASHBOARD
// ══════════════════════════════════════════════

void showDashboard() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("= SMPS AI MONITOR =");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // Voltage
  display.setCursor(0, 12);
  display.print("V:");
  display.print(gVoltage, 1);
  display.print("V");
  drawBar(45, 12, 60, 7, gVoltage, 15.0);

  // Current
  display.setCursor(0, 22);
  display.print("I:");
  display.print(gCurrent, 2);
  display.print("A");
  drawBar(45, 22, 60, 7, gCurrent, 5.0);

  // Temperature
  display.setCursor(0, 32);
  display.print("T:");
  display.print(gTemp, 1);
  display.print("C");
  drawBar(45, 32, 60, 7, gTemp, 80.0);

  // Power
  display.setCursor(0, 42);
  display.print("PWR:");
  display.print(gPower, 1);
  display.print("W");

  display.drawLine(0, 52, 127, 52, SSD1306_WHITE);

  // AI Status
  display.setCursor(0, 55);
  display.print("AI: ");
  display.print(gClass);
  display.print(" ");
  display.print(gConf * 100, 1);
  display.print("%");

  display.display();
}

// ══════════════════════════════════════════════
// ✅ FIX 3: FAULT SCREEN — CLASS NAMES CORRECTED
// Model uses: "Overtemp" NOT "Overtemperature"
// ══════════════════════════════════════════════

void showFaultScreen() {
  if (millis() - lastBlink > 400) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  display.clearDisplay();

  if (blinkState) {
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(15, 5);
  display.println("!! FAULT DETECTED !!");
  display.drawLine(0, 15, 127, 15, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(5, 19);

  // ✅ FIX 3: Correct class names from model
  if (gClass == "Overcurrent") {
    display.println("OVERCURR");
  } else if (gClass == "Overvoltage") {
    display.println("OVERVOLT");
  } else if (gClass == "Overtemp") {   // ✅ Was "Overtemperature" (WRONG)
    display.println("OVERTEMP");
  } else {
    display.println(gClass);
  }

  display.setTextSize(1);
  display.setCursor(5, 40);

  // ✅ FIX 3: Correct class name in value display
  if (gClass == "Overcurrent") {
    display.print("Current: ");
    display.print(gCurrent, 2);
    display.print(" A");
  } else if (gClass == "Overvoltage") {
    display.print("Voltage: ");
    display.print(gVoltage, 1);
    display.print(" V");
  } else if (gClass == "Overtemp") {   // ✅ Fixed
    display.print("Temp: ");
    display.print(gTemp, 1);
    display.print(" C");
  }

  display.drawLine(0, 50, 127, 50, SSD1306_WHITE);
  display.setCursor(5, 54);
  if (blinkState) {
    display.println("  LOAD DISCONNECTED");
  }

  display.display();
}

// ══════════════════════════════════════════════
// BOOT SCREEN
// ══════════════════════════════════════════════

void bootScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(20, 5);
  display.println("EDGE AI SMPS");
  display.setCursor(15, 16);
  display.println("FAULT DETECTION");
  display.drawLine(0, 26, 127, 26, SSD1306_WHITE);

  display.setCursor(25, 30);
  display.println("Initializing...");

  for (int i = 0; i <= 110; i += 5) {
    display.fillRect(10, 45, i, 8, SSD1306_WHITE);
    display.display();
    delay(40);
  }

  display.setCursor(35, 56);
  display.println("Ready!");
  display.display();
  delay(1000);
}

// ══════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("════════════════════════════════");
  Serial.println("  Edge AI SMPS Fault Detection  ");
  Serial.println("  by Jay Poriya (jayporiya03)   ");
  Serial.println("════════════════════════════════");

  pinMode(RELAY_PIN,   OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  // ✅ FIX 2: Safe defaults — Active LOW relay
  digitalWrite(RELAY_PIN,   LOW);   // Relay ON at boot
  digitalWrite(BUZZER_PIN,  LOW);
  digitalWrite(LED_RED_PIN, LOW);

  sensors.begin();
  Serial.print("DS18B20 found: ");
  Serial.println(sensors.getDeviceCount());

  // ✅ FIX 4: Added attenuation for correct ADC range
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found!");
    while (true) { delay(500); }
  }

  display.clearDisplay();
  display.display();
  delay(100);

  bootScreen();
  Serial.println("✅ System Ready!");
}

// ══════════════════════════════════════════════
// MAIN LOOP
// ══════════════════════════════════════════════

void loop() {

  // Step 1: Read sensors
  gVoltage = readVoltage();
  gCurrent = readCurrent();
  gTemp    = readTemperature();
  gPower   = gVoltage * gCurrent;

  Serial.printf("V:%.2fV  I:%.2fA  T:%.1fC  P:%.2fW\n",
    gVoltage, gCurrent, gTemp, gPower);

  // ✅ FIX 5: Read fresh sensor values every sample
  // (was reusing same gVoltage/gCurrent/gTemp)
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += 3) {
    features[i]     = readVoltage();
    features[i + 1] = readCurrent();
    features[i + 2] = readTemperature();
    delay(10); // 100Hz
  }

  // Step 2: Create signal
  signal_t signal;
  int err = numpy::signal_from_buffer(
    features,
    EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
    &signal
  );
  if (err != 0) {
    Serial.print("❌ Signal error: ");
    Serial.println(err);
    return;
  }

  // Step 3: Run inference
  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  if (res != EI_IMPULSE_OK) {
    Serial.print("❌ Inference error: ");
    Serial.println(res);
    return;
  }

  // Step 4: Get best class
  float maxScore = 0;
  String detectedClass = "";

  Serial.println("─── Results ─────────────────");
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.printf("  %s: %.2f%%\n",
      result.classification[i].label,
      result.classification[i].value * 100);

    if (result.classification[i].value > maxScore) {
      maxScore      = result.classification[i].value;
      detectedClass = result.classification[i].label;
    }
  }

  gClass = detectedClass;
  gConf  = maxScore;

  Serial.printf("✅ %s (%.1f%%)\n",
    gClass.c_str(), gConf * 100);
  Serial.println("──────────────────────────────");

  // Step 5: Handle outputs
  handleOutputs(gClass);

  // Step 6: Show correct screen
  if (isFault) {
    showFaultScreen();
  } else {
    showDashboard();
  }

  delay(300);
}
