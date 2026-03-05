#include <Arduino.h>

// ============================================================
// Lab 2.1 - ADC/DAC investigation
//
// Wiring:
//   3.3V --> [Photoresistor] --> GPIO_ADC_PIN --> [10kOhm] --> GND
//   GPIO_LED_PIN --> [100Ohm] --> [LED] --> GND
//
// NOTE: ESP32-S3 has no hardware DAC.
//       We use PWM (8-bit, 0-255) instead of dacWrite().
// ============================================================

#define ADC_PIN     4     // GPIO4  - ADC input (photoresistor)
#define LED_PIN     16    // GPIO16 - PWM output (LED)
#define PWM_CHANNEL 0
#define PWM_FREQ    5000  // 5 kHz
#define PWM_RES     8     // 8-bit resolution (0-255)

#define ADC_MAX     4095

void setup()
{
  Serial.begin(115200);

  // LED PWM setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);

  delay(3000); // time for serial monitor to connect

  Serial.println("ADC   : GPIO 4  (photoresistor)");
  Serial.println("LED   : GPIO 16 (PWM brightness)");
  Serial.println("-------------------------------");
  Serial.println("Light Level  | ADC (0-4095) | PWM (0-255)");
  Serial.println("-------------|-------------|------------");
}

void loop()
{
  // Read ADC value from photoresistor (12-bit: 0-4095)
  int adcValue = analogRead(ADC_PIN);

  // Map ADC value to PWM range (0-255)
  int pwmValue = map(adcValue, 0, ADC_MAX, 0, 255);

  // Write PWM to LED
  ledcWrite(PWM_CHANNEL, pwmValue);

  // Classify light level
  const char* lightLevel;
  if (adcValue < 500)        lightLevel = "Very Dark  ";
  else if (adcValue < 1500)  lightLevel = "Dark       ";
  else if (adcValue < 2500)  lightLevel = "Normal     ";
  else if (adcValue < 3500)  lightLevel = "Bright     ";
  else                       lightLevel = "Very Bright";

  Serial.print(lightLevel);
  Serial.print(" | ");
  Serial.print(adcValue);
  Serial.print("          | ");
  Serial.println(pwmValue);

  delay(500);
}