/**
 * LAB 4.3 -- Kalman Filter on DS18B20 temperature data
 *
 * Wiring:
 *   DS18B20 DATA -> GPIO4  (4.7 kOhm pull-up to 3.3V required!)
 *   DS18B20 VCC  -> 3.3V
 *   DS18B20 GND  -> GND
 *
 * Steps:
 *  1. Collect SAMPLE_COUNT readings at INTERVAL_MS intervals
 *  2. Apply 1-D Kalman filter to the collected array (offline pass)
 *  3. Print raw vs filtered comparison and total error reduction
 *
 * Kalman filter parameters:
 *   Q -- process noise covariance  (how much the real temp can change per step)
 *   R -- measurement noise covariance (how noisy the sensor is)
 *   Smaller Q/R ratio -> filter trusts the model more (slower response)
 *   Larger  Q/R ratio -> filter trusts measurements more (faster response)
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

#define ONE_WIRE_BUS   4
#define SAMPLE_COUNT   100
#define INTERVAL_MS    1000

// --- Kalman tuning parameters -------------------------------------------------
#define KF_Q   0.022f   // process noise
#define KF_R   0.617f   // measurement noise (sensor noise variance)

OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

float raw[SAMPLE_COUNT];
float filtered[SAMPLE_COUNT];

// Simple scalar (1-D) Kalman filter applied to a pre-collected array
void kalmanFilter(const float *in, float *out, int n) {
    // Initial state: trust the first measurement
    float x_est = in[0];
    float P     = 1.0f;

    for (int i = 0; i < n; i++) {
        // 1. Predict
        float x_pred = x_est;
        float P_pred = P + KF_Q;

        // 2. Update (Kalman gain)
        float K = P_pred / (P_pred + KF_R);

        // 3. Correct
        x_est = x_pred + K * (in[i] - x_pred);
        P     = (1.0f - K) * P_pred;

        out[i] = x_est;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("  LAB 4.3 -- Kalman Filter  |  DS18B20");
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
            t = (i > 0) ? raw[i - 1] : 25.0f;
            Serial.printf("%3d,  ERROR  (using %.4f)\n", i, t);
        } else {
            Serial.printf("%3d,  %.4f\n", i, t);
        }

        raw[i] = t;
        delay(INTERVAL_MS);
    }

    // --- Phase 2: apply Kalman filter ----------------------------------------
    kalmanFilter(raw, filtered, SAMPLE_COUNT);

    // --- Phase 3: print comparison + error stats -----------------------------
    Serial.println();
    Serial.println("==============================================");
    Serial.printf( "  Kalman filter  (Q=%.3f, R=%.3f)\n", KF_Q, KF_R);
    Serial.println("==============================================");
    Serial.println("idx,  raw_C,     kalman_C,   delta");

    // Use the mean of raw as a simple "true" reference for error estimation
    float mean = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) mean += raw[i];
    mean /= SAMPLE_COUNT;

    float err_raw    = 0;
    float err_kalman = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        float delta = filtered[i] - raw[i];
        Serial.printf("%3d,  %8.4f,  %8.4f,  %+.4f\n",
                      i, raw[i], filtered[i], delta);

        err_raw    += fabsf(raw[i]      - mean);
        err_kalman += fabsf(filtered[i] - mean);
    }

    Serial.println();
    Serial.printf("Mean (reference):          %.4f C\n", mean);
    Serial.printf("Total deviation raw:       %.4f\n", err_raw);
    Serial.printf("Total deviation Kalman:    %.4f\n", err_kalman);
    if (err_raw > 0) {
        int reduction = 100 - (int)((err_kalman / err_raw) * 100.0f);
        Serial.printf("Noise reduction:           %d%%\n", reduction);
    }

    Serial.println("\nDone. Reset to run again.");
}

void loop() {
    // all work is done in setup()
}
