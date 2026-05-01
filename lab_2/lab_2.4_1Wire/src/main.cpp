/**
 * LAB 2.4 -- 1-Wire Protocol on ESP32-S3
 *
 * Sensor:
 *   DS18B20  -- digital temperature sensor, 1-Wire bus
 *
 * Wiring:
 *   DS18B20  DATA  -> GPIO4   (4.7 kOhm pull-up to 3.3V required!)
 *   DS18B20  VCC   -> 3.3V
 *   DS18B20  GND   -> GND
 *
 * 1-Wire key facts:
 *   - Single wire for data + power (parasitic mode) or separate VCC
 *   - Each device has unique 64-bit ROM address (8-byte serial number)
 *   - Master initiates all communication; devices answer by address
 *   - Multiple sensors can share the same bus wire
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Pin definitions ---------------------------------------------------------
#define ONE_WIRE_BUS  4    // DS18B20 data pin

// --- Objects -----------------------------------------------------------------
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// --- Helpers -----------------------------------------------------------------
void printRomAddress(const uint8_t *addr) {
    for (uint8_t i = 0; i < 8; i++) {
        if (addr[i] < 0x10) Serial.print('0');
        Serial.print(addr[i], HEX);
        if (i < 7) Serial.print(':');
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================================");
    Serial.println("  LAB 2.4 -- 1-Wire Protocol  |  DS18B20 Demo");
    Serial.println("================================================");
    Serial.println();

    // --- 1-Wire bus init -----------------------------------------------------
    ds18b20.begin();
    Serial.printf("1-Wire bus pin: GPIO%d\n", ONE_WIRE_BUS);

    uint8_t count = ds18b20.getDeviceCount();
    Serial.printf("DS18B20 sensors found on bus: %d\n\n", count);

    if (count == 0) {
        Serial.println("ERROR: No DS18B20 found!");
        Serial.println("  Check: DATA -> GPIO4, pull-up 4.7kOhm to 3.3V");
    } else {
        // Print ROM address of each found sensor
        Serial.println("ROM addresses (64-bit unique IDs):");
        for (uint8_t i = 0; i < count; i++) {
            DeviceAddress addr;
            if (ds18b20.getAddress(addr, i)) {
                Serial.printf("  Sensor [%d]: ", i);
                printRomAddress(addr);
                // Byte 0 = family code: 0x28 = DS18B20, 0x10 = DS18S20
                Serial.printf("  (family: 0x%02X)\n", addr[0]);
            }
        }
        // Set resolution: 9, 10, 11, or 12 bits (12 = 0.0625 C precision)
        ds18b20.setResolution(12);
        Serial.println("\nResolution set to 12-bit (0.0625 C)");
    }

    Serial.println();
    Serial.println("------------------------------------------------");
    Serial.println("Starting measurements every 2 seconds...");
    Serial.println("------------------------------------------------");
    Serial.println();
}

void loop() {
    Serial.println("+-- 1-Wire  DS18B20 -----------------------------");

    ds18b20.requestTemperatures();  // broadcasts CONVERT T command to all devices

    uint8_t count = ds18b20.getDeviceCount();
    if (count == 0) {
        Serial.println("|  No sensors detected!");
    } else {
        for (uint8_t i = 0; i < count; i++) {
            float tempC = ds18b20.getTempCByIndex(i);

            DeviceAddress addr;
            ds18b20.getAddress(addr, i);

            Serial.printf("|  Sensor [%d]  ROM: ", i);
            printRomAddress(addr);
            Serial.println();

            if (tempC == DEVICE_DISCONNECTED_C) {
                Serial.println("|    -> ERROR: sensor disconnected");
            } else {
                Serial.printf("|    -> Temperature: %.4f C  (%.4f F)\n",
                              tempC, ds18b20.toFahrenheit(tempC));
            }
        }
    }

    Serial.println("+------------------------------------------------");
    Serial.println();

    delay(2000);
}