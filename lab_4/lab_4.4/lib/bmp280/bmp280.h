#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Arduino Wire-based BMP280 driver.
 *
 * Works with all clones regardless of chip ID (0x56, 0x57, 0x58, 0x60).
 * Uses Bosch compensation formulas from the BMP280 datasheet.
 */
class BMP280 {
public:
    explicit BMP280(uint8_t addr = 0x76);

    /**
     * @brief Initialize the sensor on the given Wire bus.
     * @return true if the sensor responded (any non-zero chip ID).
     */
    bool begin(TwoWire &wire = Wire);

    /** @return Temperature in °C */
    float readTemperature();

    /** @return Pressure in hPa */
    float readPressure();

    /** @return Raw chip ID byte (for diagnostics) */
    uint8_t chipID();

private:
    uint8_t   _addr;
    TwoWire  *_wire;
    int32_t   _t_fine = 0;

    struct {
        uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
        uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3;
        int16_t  dig_P4; int16_t dig_P5; int16_t dig_P6;
        int16_t  dig_P7; int16_t dig_P8; int16_t dig_P9;
    } _calib;

    uint8_t  read8(uint8_t reg);
    uint16_t read16LE(uint8_t reg);
    void     write8(uint8_t reg, uint8_t val);
    void     readCalib();
    int32_t  readRawTemp();
    int32_t  readRawPressure();
};
