#include <Arduino.h>

// --- Pin definitions (ESP32-S3) ---
#define BUTTON1_PIN        4
#define BUTTON2_PIN        5
#define LED1_PIN           6
#define LED2_PIN           7
#define LED3_PIN           15
#define PHOTORESISTOR_PIN  3

// --- PWM config ---
#define PWM_CHANNEL    0
#define PWM_FREQ       5000
#define PWM_RESOLUTION 8

// --- Global state (volatile for ISR access) ---
volatile bool button1Pressed = false;
volatile bool button2Pressed = false;
volatile unsigned long lastISR1Time = 0;
volatile unsigned long lastISR2Time = 0;

int pressCount = 0;
bool led2State = false;
int brightnessStep = 0;

// --- ISR handlers (debounce 200ms) ---
void IRAM_ATTR isr_button1() {
  unsigned long now = millis();
  if (now - lastISR1Time > 200) {
    button1Pressed = true;
    lastISR1Time = now;
  }
}

void IRAM_ATTR isr_button2() {
  unsigned long now = millis();
  if (now - lastISR2Time > 200) {
    button2Pressed = true;
    lastISR2Time = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Lab 3.1: ESP32 Interrupts ===\n");

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED3_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), isr_button1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), isr_button2, FALLING);

  Serial.println("Interrupts registered. Waiting for button presses...\n");
}

void loop() {
  // Button 1: read photoresistor + blink LED1
  if (button1Pressed) {
    button1Pressed = false;

    int photoValue = analogRead(PHOTORESISTOR_PIN);
    float voltage = photoValue * (3.3 / 4095.0);

    Serial.println("─────────────────────────────────────");
    Serial.println("[Button 1] Interrupt triggered!");
    Serial.printf("  Photoresistor: RAW = %d, Voltage = %.2f V\n", photoValue, voltage);
    Serial.printf("  Light level: %s\n",
      photoValue > 3000 ? "Dark" :
      photoValue > 1500 ? "Medium" : "Bright");

    for (int i = 0; i < 3; i++) {
      digitalWrite(LED1_PIN, HIGH);
      delay(150);
      digitalWrite(LED1_PIN, LOW);
      delay(150);
    }
    Serial.println("  LED1: blink done.\n");
  }

  // Button 2: counter + toggle LED2 + adjust LED3 brightness via PWM
  if (button2Pressed) {
    button2Pressed = false;

    pressCount++;
    Serial.println("─────────────────────────────────────");
    Serial.printf("[Button 2] Interrupt! Press count: %d\n", pressCount);

    led2State = !led2State;
    digitalWrite(LED2_PIN, led2State ? HIGH : LOW);
    Serial.printf("  LED2: %s\n", led2State ? "ON" : "OFF");

    brightnessStep = (brightnessStep + 1) % 6;
    int brightness = brightnessStep * 51;
    ledcWrite(PWM_CHANNEL, brightness);
    Serial.printf("  LED3 brightness: %d/255 (%d%%)\n\n", brightness, brightnessStep * 20);
  }

  delay(10);
}