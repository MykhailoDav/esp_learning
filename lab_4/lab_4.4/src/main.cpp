/**
 * LAB 4.4 -- SPIFFS File System on ESP32-S3
 *
 * Sensors:
 *   DS18B20      1-Wire  -> GPIO4   (4.7 kOhm pull-up to 3.3V)
 *   BMP280       I2C     -> SDA=GPIO41, SCL=GPIO42, addr=0x76
 *   Photoresistor ADC    -> GPIO1   (10 kOhm divider: 3.3V->[10k]->GPIO1->[photo]->GND)
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "bmp280.h"
#include "esp_spiffs.h"   // esp_spiffs_format()

// Піни для підключення
#define ONE_WIRE_BUS  4
#define LDR_PIN       1
#define I2C_SDA       41
#define I2C_SCL       42

#define SAMPLE_COUNT  10
#define INTERVAL_MS   5000UL
#define DATA_FILE     "/sensors_data.csv"

BMP280 bmp(0x76);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

struct Sample {
    unsigned long t_s;
    float temp_ds, temp_bmp, pressure;
    int   light;
} samples[SAMPLE_COUNT];

int           sampleIdx      = 0;
unsigned long lastSampleTime = 0;

// --- Format via ESP-IDF API, then mount clean --------------------------------
void initSPIFFS() {
    // esp_spiffs_format() does its own erase+format+deinit internally.
    // Using it avoids the "failed-mount → dirty internal state" problem
    // that happens when begin(true) tries to mount an erased partition first.
    Serial.println("[SPIFFS] Formatting...");
    esp_err_t err = esp_spiffs_format(NULL);   // NULL = default partition label
    if (err != ESP_OK) {
        Serial.printf("FATAL: format failed: %s\n", esp_err_to_name(err));
        while (true) delay(1000);
    }
    Serial.println("[SPIFFS] Format OK");

    if (!SPIFFS.begin(false)) {   // false = partition is already formatted
        Serial.println("FATAL: SPIFFS mount failed");
        while (true) delay(1000);
    }
    Serial.printf("[SPIFFS] Mounted.  Total: %u B  Used: %u B\n\n",
                  SPIFFS.totalBytes(), SPIFFS.usedBytes());
}

// --- Write all samples to SPIFFS, then read back ----------------------------
void saveAndPrint() {
    // Build entire CSV in RAM — avoids File.printf() 27-byte buffer truncation
    String csv = "Time(s),Temp_DS(C),Temp_BMP(C),Pressure(hPa),Light_ADC\n";
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        csv += String(samples[i].t_s)           + "," +
               String(samples[i].temp_ds,  2)   + "," +
               String(samples[i].temp_bmp, 2)   + "," +
               String(samples[i].pressure, 2)   + "," +
               String(samples[i].light)          + "\n";
    }

    // Open "w+" = create/truncate + allow read on the SAME handle.
    // Avoids close→reopen which requires SPIFFS to find data pages in flash
    // (those pages may still be in the write cache and not yet committed).
    FILE* fp = fopen("/spiffs" DATA_FILE, "w+");
    if (!fp) { Serial.println("ERROR: fopen(w+) failed"); return; }

    size_t written = fwrite(csv.c_str(), 1, csv.length(), fp);
    fflush(fp);
    Serial.printf("[SPIFFS] Written %u/%u bytes. Used: %u B\n",
                  written, csv.length(), SPIFFS.usedBytes());

    // Seek to start and read back from the same handle — no flush needed
    rewind(fp);

    Serial.println("\n==================================================");
    Serial.println("   ПОТОЧНИЙ ВМІСТ ФАЙЛУ " DATA_FILE);
    Serial.println("==================================================");

    int c;
    while ((c = fgetc(fp)) != EOF) {
        Serial.write((uint8_t)c);
    }
    fclose(fp);

    Serial.println("\n==================================================");
    Serial.println("Легенда: Час | DS18B20 | BMP280 | Тиск | Освітленість");
    Serial.println("==================================================");
    Serial.println("Готово! Перезавантажте плату для повтору.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    initSPIFFS();

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!bmp.begin(Wire)) {
        Serial.println("Не знайдено BMP280");
    } else {
        Serial.printf("BMP280 готовий. Chip ID: 0x%02X\n", bmp.chipID());
    }

    sensors.begin();
    analogReadResolution(12);

    Serial.printf("Збір %d замірів кожні %lu мс...\n", SAMPLE_COUNT, INTERVAL_MS);
    Serial.println("idx | time_s | DS18B20 | BMP280 | pressure | light");
    Serial.println("----|--------|---------|--------|----------|------");

    lastSampleTime = millis() - INTERVAL_MS;
}

void loop() {
    if (sampleIdx >= SAMPLE_COUNT) return;

    if (millis() - lastSampleTime < INTERVAL_MS) return;
    lastSampleTime = millis();

    sensors.requestTemperatures();
    float temp_ds  = sensors.getTempCByIndex(0);
    float temp_bmp = bmp.readTemperature();
    float pressure = bmp.readPressure();
    int   light    = analogRead(LDR_PIN);
    unsigned long t_s = millis() / 1000;

    samples[sampleIdx] = { t_s, temp_ds, temp_bmp, pressure, light };

    Serial.printf("%3d | %6lu | %7.2f | %6.2f | %8.2f | %5d\n",
                  sampleIdx, t_s, temp_ds, temp_bmp, pressure, light);
    sampleIdx++;

    if (sampleIdx >= SAMPLE_COUNT) {
        saveAndPrint();
    }
}
