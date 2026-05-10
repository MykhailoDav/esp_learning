/**
 * LAB 4.4 -- SPIFFS File System on ESP32-S3
 *
 * Sensors:
 *   DS18B20      1-Wire  -> GPIO4   (4.7 kOhm pull-up to 3.3V)
 *   BMP280       I2C     -> SDA=GPIO41, SCL=GPIO42, addr=0x76
 *   Photoresistor ADC    -> GPIO1   (10 kOhm divider: 3.3V->[10k]->GPIO1->[photo]->GND)
 *
 * Root cause of all SPIFFS corruption: SPIFFS.format() only rewrites metadata
 * headers but does NOT do a hardware sector erase. Old data bits (0) remain in
 * NOR-flash pages and bleed through into new writes (NOR can only 1->0, not 0->1
 * without a sector erase). Fix: esp_partition_erase_range() performs the real
 * sector erase before SPIFFS.begin().
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "bmp280.h"
#include "esp_partition.h"   // for esp_partition_erase_range

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

// --- Hardware-erase the SPIFFS partition, then mount -------------------------
void initSPIFFS() {
    const esp_partition_t* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);

    if (part == NULL) {
        Serial.println("FATAL: SPIFFS partition not found");
        while (true) delay(1000);
    }

    Serial.printf("[SPIFFS] Erasing partition (0x%08x, %u B)...\n",
                  part->address, part->size);
    esp_err_t err = esp_partition_erase_range(part, 0, part->size);
    if (err != ESP_OK) {
        Serial.printf("FATAL: erase failed: %s\n", esp_err_to_name(err));
        while (true) delay(1000);
    }
    Serial.println("[SPIFFS] Partition erased OK");

    if (!SPIFFS.begin(true)) {   // true = format fresh after erase
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

    // Write the whole String at once with write() — no format-buffer limit
    File f = SPIFFS.open(DATA_FILE, FILE_WRITE);
    if (!f) { Serial.println("ERROR: cannot create file!"); return; }
    size_t written = f.write((const uint8_t*)csv.c_str(), csv.length());
    f.close();
    Serial.printf("[SPIFFS] Written %d/%d bytes. Used: %u B\n",
                  written, csv.length(), SPIFFS.usedBytes());

    Serial.println("\n==================================================");
    Serial.println("   ПОТОЧНИЙ ВМІСТ ФАЙЛУ " DATA_FILE);
    Serial.println("==================================================");

    f = SPIFFS.open(DATA_FILE, FILE_READ);
    if (!f) { Serial.println("ERROR: cannot open file!"); return; }

    f.setTimeout(0);
    char buf[64];
    int  n;
    while ((n = f.readBytes(buf, 63)) > 0) {
        buf[n] = '\0';
        Serial.print(buf);
    }
    f.close();

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
