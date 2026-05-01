/**
 * LAB 2.5 -- Complex Sensor Demo on ESP32-S3
 *
 * Sensors:
 *   DS18B20      -- 1-Wire digital temperature sensor
 *   BMP280       -- I2C barometric pressure + temperature sensor
 *   Photoresistor -- analog light sensor via ADC
 *
 * Wiring:
 *   DS18B20   DATA  -> GPIO4   (5.1 kOhm pull-up to 3.3V)
 *   DS18B20   VCC   -> 3.3V
 *   DS18B20   GND   -> GND
 *
 *   BMP280    SDA   -> GPIO41
 *   BMP280    SCL   -> GPIO42
 *   BMP280    VCC   -> 3.3V
 *   BMP280    GND   -> GND
 *   BMP280    addr  -> GND  (I2C address = 0x76)
 *
 *   Photoresistor: 3.3V -> 10kOhm -> GPIO1 -> photoresistor -> GND
 *   (higher light = lower resistance = higher voltage on GPIO1)
 */

#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "bmp280.h"

// --- Pin definitions ---------------------------------------------------------
#define ONE_WIRE_PIN   4    // DS18B20 data
#define I2C_SDA        41
#define I2C_SCL        42
#define PHOTO_PIN      1    // ADC -- photoresistor divider

// --- Objects -----------------------------------------------------------------
OneWire           oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
BMP280            bmp(0x76);

// --- Helpers -----------------------------------------------------------------
void printRomAddress(const uint8_t *addr) {
    for (uint8_t i = 0; i < 8; i++) {
        if (addr[i] < 0x10) Serial.print('0');
        Serial.print(addr[i], HEX);
        if (i < 7) Serial.print(':');
    }
}

// =============================================================================
// setup()
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================================");
    Serial.println("  LAB 2.5 -- Complex Sensor Demo");
    Serial.println("  DS18B20 | BMP280 | Photoresistor");
    Serial.println("================================================");
    Serial.println();

    // --- 1-Wire / DS18B20 ----------------------------------------------------
    ds18b20.begin();
    Serial.printf("[1-Wire] Bus pin: GPIO%d\n", ONE_WIRE_PIN);
    uint8_t count = ds18b20.getDeviceCount();
    Serial.printf("[1-Wire] DS18B20 found: %d\n", count);
    if (count > 0) {
        for (uint8_t i = 0; i < count; i++) {
            DeviceAddress addr;
            if (ds18b20.getAddress(addr, i)) {
                Serial.printf("  Sensor [%d] ROM: ", i);
                printRomAddress(addr);
                Serial.println();
            }
        }
        ds18b20.setResolution(12);
        Serial.println("  Resolution: 12-bit (0.0625 C)");
    } else {
        Serial.println("  ERROR: no DS18B20 detected -- check wiring & pull-up");
    }
    Serial.println();

    // --- I2C / BMP280 --------------------------------------------------------
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("[I2C]    SDA=GPIO%d  SCL=GPIO%d\n", I2C_SDA, I2C_SCL);

    // I2C bus scan -- helps diagnose wiring issues
    Serial.println("  Scanning I2C bus...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  -> device found at 0x%02X\n", addr);
            found++;
        }
    }
    if (found == 0) Serial.println("  -> no I2C devices found! Check SDA/SCL wiring.");
    if (!bmp.begin(Wire)) {
        Serial.println("  ERROR: BMP280 not found -- check wiring & address");
    } else {
        Serial.printf("  BMP280 found, chip ID: 0x%02X\n", bmp.chipID());
    }
    Serial.println();

    // --- ADC / Photoresistor -------------------------------------------------
    Serial.printf("[ADC]    Photoresistor pin: GPIO%d\n", PHOTO_PIN);
    analogReadResolution(12);   // 0-4095
    Serial.println();

    Serial.println("------------------------------------------------");
    Serial.println("Readings every 2 seconds");
    Serial.println("------------------------------------------------");
    Serial.println();
}

// =============================================================================
// loop()
// =============================================================================
void loop() {
    // --- DS18B20 -------------------------------------------------------------
    Serial.println("+-- DS18B20  (1-Wire, GPIO4) --------------------");
    ds18b20.requestTemperatures();
    uint8_t count = ds18b20.getDeviceCount();
    if (count == 0) {
        Serial.println("|  No sensor detected");
    } else {
        for (uint8_t i = 0; i < count; i++) {
            float t = ds18b20.getTempCByIndex(i);
            if (t == DEVICE_DISCONNECTED_C) {
                Serial.printf("|  Sensor [%d]: DISCONNECTED\n", i);
            } else {
                Serial.printf("|  Sensor [%d]: %.4f C\n", i, t);
            }
        }
    }

    // --- BMP280 --------------------------------------------------------------
    Serial.println("+-- BMP280   (I2C, SDA=41 SCL=42) --------------");
    float bmpTemp = bmp.readTemperature();
    float pressure = bmp.readPressure();        // already hPa
    float altitude = 44330.0f * (1.0f - powf(pressure / 1013.25f, 0.1903f));
    Serial.printf("|  Temperature: %.2f C\n", bmpTemp);
    Serial.printf("|  Pressure:    %.2f hPa\n", pressure);
    Serial.printf("|  Altitude:    %.1f m\n", altitude);

    // --- Photoresistor -------------------------------------------------------
    Serial.println("+-- Photoresistor  (ADC, GPIO1) -----------------");
    int raw = analogRead(PHOTO_PIN);                // 0-4095
    float voltage = raw * 3.3f / 4095.0f;
    // Light level as percentage: more light = more voltage (for this divider)
    float lightPct = (voltage / 3.3f) * 100.0f;
    Serial.printf("|  ADC raw:   %d\n", raw);
    Serial.printf("|  Voltage:   %.3f V\n", voltage);
    Serial.printf("|  Light:     %.1f %%\n", lightPct);

    Serial.println("+------------------------------------------------");
    Serial.println();

    delay(2000);
}