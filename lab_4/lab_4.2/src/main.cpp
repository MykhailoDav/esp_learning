/**
 * LAB 4.2 -- Median Filter on DS18B20 temperature data
 *
 * Wiring (same as lab 2.4):
 *   DS18B20 DATA -> GPIO4  (4.7 kOhm pull-up to 3.3V required!)
 *   DS18B20 VCC  -> 3.3V
 *   DS18B20 GND  -> GND
 *
 * Steps:
 *  1. Collect SAMPLE_COUNT readings at INTERVAL_MS intervals
 *  2. Apply 1-D median filter with WINDOW_SIZE to the collected array
 *  3. Print raw vs filtered comparison over Serial
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <algorithm>

#define ONE_WIRE_BUS   4
#define SAMPLE_COUNT   100
#define INTERVAL_MS    1000
#define WINDOW_SIZE    5    // must be odd; larger = smoother but more lag

OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

float raw[SAMPLE_COUNT];
float filtered[SAMPLE_COUNT];

// 1-D median filter with clamped (edge-repeat) boundary handling
void medianFilter(const float *in, float *out, int n, int win) {
    int half = win / 2;
    float buf[WINDOW_SIZE];

    for (int i = 0; i < n; i++) {
        int count = 0;
        for (int k = -half; k <= half; k++) {
            int idx = i + k;
            if (idx < 0)  idx = 0;
            if (idx >= n) idx = n - 1;
            buf[count++] = in[idx];
        }
        std::sort(buf, buf + count);
        out[i] = buf[count / 2];
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("  LAB 4.2 -- Median Filter  |  DS18B20");
    Serial.println("==============================================");

    sensor.begin();
    uint8_t cnt = sensor.getDeviceCount();
    Serial.printf("DS18B20 sensors found: %d\n\n", cnt);

    if (cnt == 0) {
        Serial.println("ERROR: No DS18B20 found! Check wiring (GPIO4, 4.7k pull-up).");
        while (true) delay(1000);
    }
    sensor.setResolution(12);   // 0.0625 C precision

    // --- Phase 1: collect raw data -------------------------------------------
    Serial.printf("Collecting %d samples, interval %d ms ...\n\n", SAMPLE_COUNT, INTERVAL_MS);
    Serial.println("idx,  raw_C");

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sensor.requestTemperatures();
        float t = sensor.getTempCByIndex(0);

        if (t == DEVICE_DISCONNECTED_C) {
            // sensor error: repeat last valid reading so the array stays full
            t = (i > 0) ? raw[i - 1] : 0.0f;
            Serial.printf("%3d,  ERROR  (using %.4f)\n", i, t);
        } else {
            Serial.printf("%3d,  %.4f\n", i, t);
        }

        raw[i] = t;
        delay(INTERVAL_MS);
    }

    // --- Phase 2: apply median filter ----------------------------------------
    medianFilter(raw, filtered, SAMPLE_COUNT, WINDOW_SIZE);

    // --- Phase 3: print comparison -------------------------------------------
    Serial.println();
    Serial.println("==============================================");
    Serial.printf( "  Median filter applied  (window = %d)\n", WINDOW_SIZE);
    Serial.println("==============================================");
    Serial.println("idx,  raw_C,     filtered_C,  delta");

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        Serial.printf("%3d,  %8.4f,  %8.4f,    %+.4f\n",
                      i, raw[i], filtered[i], filtered[i] - raw[i]);
    }

    Serial.println("\nDone. Reset to run again.");
}

void loop() {
    // all work is done in setup()
}
