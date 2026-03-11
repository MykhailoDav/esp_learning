#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include "bmp280.h"
#include "ssd1306.h"

// ===== I2C pins (GPIO41 = SDA, GPIO42 = SCL) =====
#define I2C_SDA 41
#define I2C_SCL 42

// ===== Sensor objects =====
BMP280          bmp(0x76);
Adafruit_HTU21DF htu21;
SSD1306         oled(0x3C, 128, 32);  // 128×32 display at 0x3C

// ===== State flags =====
bool bmp_ok  = false;
bool htu_ok  = false;
bool oled_ok = false;

// ===== Sensor readings =====
float bmp_temp     = 0;
float bmp_pressure = 0;
float htu_temp     = 0;
float htu_humidity = 0;

// ===== I2C scanner =====
void scanI2C() {
  Serial.println("\n===== Сканування I2C шини =====");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("Знайдено: %d\n", found);
}

// ===== Init / re-init =====
void initDevices() {
  Serial.println("\n===== Ініціалізація =====");

  if (!bmp_ok) {
    bmp_ok = bmp.begin(Wire);
    Serial.printf("BMP280 chip ID: 0x%02X\n", bmp.chipID());
    Serial.println(bmp_ok ? "✓ BMP280 OK" : "❌ BMP280 FAIL");
  }
  if (!htu_ok) {
    htu_ok = htu21.begin();
    Serial.println(htu_ok ? "✓ HTU21 OK" : "❌ HTU21 FAIL");
  }
  if (!oled_ok) {
    oled_ok = oled.begin(Wire);
    Serial.println(oled_ok ? "✓ SSD1306 OK" : "❌ SSD1306 FAIL");
  }
}

// ===== Update OLED display =====
void updateOLED() {
  if (!oled_ok) return;
  oled.clear();

  // Line 0 (y=0): BMP280 temperature + pressure
  if (bmp_ok) {
    oled.printf(0, 0, "T:%.1fC  P:%.0fhPa", bmp_temp, bmp_pressure);
  } else {
    oled.print(0, 0, "BMP280: N/A");
  }

  // Line 1 (y=8): HTU21 temperature + humidity
  if (htu_ok) {
    oled.printf(0, 8, "T:%.1fC  H:%.0f%%", htu_temp, htu_humidity);
  } else {
    oled.print(0, 8, "HTU21:  N/A");
  }

  // Line 2 (y=16): temperature difference
  if (bmp_ok && htu_ok) {
    oled.printf(0, 16, "dT: %.2f C", fabsf(bmp_temp - htu_temp));
  }

  // Line 3 (y=24): uptime
  oled.printf(0, 24, "t=%lus", millis() / 1000UL);

  oled.display();
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n===== LAB 2.2: I2C — BMP280 + HTU21 + SSD1306 =====");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  scanI2C();
  initDevices();
  delay(500);
}

// ===== LOOP =====
void loop() {
  // Re-try if anything failed
  if (!bmp_ok || !htu_ok || !oled_ok) {
    initDevices();
  }

  // Read BMP280
  if (bmp_ok) {
    bmp_temp     = bmp.readTemperature();
    bmp_pressure = bmp.readPressure();
  }

  // Read HTU21
  if (htu_ok) {
    htu_temp     = htu21.readTemperature();
    htu_humidity = htu21.readHumidity();
  }

  // Serial output
  Serial.printf("\n========== t=%lus ==========\n", millis() / 1000UL);
  if (bmp_ok)
    Serial.printf("[BMP280]  Temp: %.2f C  Pressure: %.2f hPa\n", bmp_temp, bmp_pressure);
  else
    Serial.println("[BMP280]  not initialized");

  if (htu_ok)
    Serial.printf("[HTU21]   Temp: %.2f C  Humidity: %.2f %%\n", htu_temp, htu_humidity);
  else
    Serial.println("[HTU21]   not initialized");

  if (bmp_ok && htu_ok)
    Serial.printf("[COMPARE] dT = %.2f C\n", fabsf(bmp_temp - htu_temp));

  // OLED output
  updateOLED();

  delay(5000);
}
