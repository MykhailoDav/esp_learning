/**
 * LAB 3.3 — Summary: Hardware Timer + Button Interrupt + BMP280
 *
 * 1) Hardware timer (5 s) → read BMP280 (temp + pressure) via I2C → blink LED1
 * 2) Button interrupt → measure hold duration → after release, light LED2
 *    for the same duration the button was held
 */

#include <Arduino.h>
#include <Wire.h>
#include "bmp280.h"

// --- Pin definitions (ESP32-S3) ---
#define I2C_SDA        41
#define I2C_SCL        42
#define BUTTON_PIN     4      // button → GND (INPUT_PULLUP)
#define LED1_PIN       6      // timer indicator LED
#define LED2_PIN       7      // button-duration LED

// --- Timer config ---
#define TIMER_INTERVAL_US  5000000  // 5 seconds

// --- Objects ---
BMP280 bmp(0x76);
hw_timer_t *timer1 = NULL;

// --- Timer flag ---
volatile bool timerFired = false;
int readingCount = 0;

// --- Button state ---
volatile bool buttonPressed  = false;
volatile bool buttonReleased = false;
volatile unsigned long pressTime   = 0;
volatile unsigned long releaseTime = 0;

// --- ISR: timer ---
void IRAM_ATTR onTimer() {
  timerFired = true;
}

// --- ISR: button (CHANGE to catch both press and release) ---
void IRAM_ATTR isrButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    // pressed (falling edge)
    pressTime = millis();
    buttonPressed = true;
  } else {
    // released (rising edge)
    releaseTime = millis();
    buttonReleased = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Lab 3.3: Timer + Button Interrupt + BMP280 ===\n");

  // --- I2C + BMP280 ---
  Wire.begin(I2C_SDA, I2C_SCL);
  if (bmp.begin(Wire)) {
    Serial.printf("BMP280 found (chip ID: 0x%02X)\n", bmp.chipID());
  } else {
    Serial.println("BMP280 not found! Check wiring.");
  }

  // --- Pins ---
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // --- Hardware timer: 5 s interval ---
  timer1 = timerBegin(0, 80, true);          // prescaler 80 → 1 µs tick
  timerAttachInterrupt(timer1, &onTimer, true);
  timerAlarmWrite(timer1, TIMER_INTERVAL_US, true);
  timerAlarmEnable(timer1);

  // --- Button interrupt (CHANGE = both edges) ---
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isrButton, CHANGE);

  Serial.println("Timer: reads BMP280 every 5 s");
  Serial.println("Button: hold and release to replay duration on LED2\n");
}

void loop() {
  // --- Timer: read BMP280 + blink LED1 ---
  if (timerFired) {
    timerFired = false;
    readingCount++;

    float temp     = bmp.readTemperature();
    float pressure = bmp.readPressure();

    Serial.println("─────────────────────────────────────");
    Serial.printf("[Timer] Reading #%d\n", readingCount);
    Serial.printf("  Temperature: %.2f °C\n", temp);
    Serial.printf("  Pressure:    %.2f hPa\n", pressure);

    // blink LED1 to indicate reading complete
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED1_PIN, HIGH);
      delay(100);
      digitalWrite(LED1_PIN, LOW);
      delay(100);
    }
    Serial.println("  LED1: blink done.\n");
  }

  // --- Button released: light LED2 for held duration ---
  if (buttonReleased) {
    buttonReleased = false;
    buttonPressed  = false;

    unsigned long duration = releaseTime - pressTime;
    if (duration > 0 && duration < 30000) {   // sanity check (max 30 s)
      Serial.println("─────────────────────────────────────");
      Serial.printf("[Button] Held for %lu ms\n", duration);
      Serial.printf("  LED2: ON for %lu ms...\n", duration);

      digitalWrite(LED2_PIN, HIGH);
      delay(duration);
      digitalWrite(LED2_PIN, LOW);

      Serial.println("  LED2: OFF\n");
    }
  }

  delay(10);
}