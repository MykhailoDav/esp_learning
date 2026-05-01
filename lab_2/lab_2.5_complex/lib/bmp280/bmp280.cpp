#include "bmp280.h"

BMP280::BMP280(uint8_t addr) : _addr(addr), _wire(nullptr) {}

// ---------- low-level I2C helpers ----------

uint8_t BMP280::read8(uint8_t reg) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->endTransmission();
    _wire->requestFrom(_addr, (uint8_t)1);
    return _wire->read();
}

uint16_t BMP280::read16LE(uint8_t reg) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->endTransmission();
    _wire->requestFrom(_addr, (uint8_t)2);
    uint16_t lo = _wire->read();
    uint16_t hi = _wire->read();
    return (hi << 8) | lo;
}

void BMP280::write8(uint8_t reg, uint8_t val) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->write(val);
    _wire->endTransmission();
}

// ---------- calibration ----------

void BMP280::readCalib() {
    _calib.dig_T1 =          read16LE(0x88);
    _calib.dig_T2 = (int16_t)read16LE(0x8A);
    _calib.dig_T3 = (int16_t)read16LE(0x8C);
    _calib.dig_P1 =          read16LE(0x8E);
    _calib.dig_P2 = (int16_t)read16LE(0x90);
    _calib.dig_P3 = (int16_t)read16LE(0x92);
    _calib.dig_P4 = (int16_t)read16LE(0x94);
    _calib.dig_P5 = (int16_t)read16LE(0x96);
    _calib.dig_P6 = (int16_t)read16LE(0x98);
    _calib.dig_P7 = (int16_t)read16LE(0x9A);
    _calib.dig_P8 = (int16_t)read16LE(0x9C);
    _calib.dig_P9 = (int16_t)read16LE(0x9E);
}

// ---------- public API ----------

uint8_t BMP280::chipID() {
    return read8(0xD0);
}

bool BMP280::begin(TwoWire &wire) {
    _wire = &wire;
    if (chipID() == 0) return false;   // no response on bus

    readCalib();
    // ctrl_meas: normal mode, osrs_t x2, osrs_p x16
    write8(0xF4, 0b01010111);
    // config: filter x16, standby 500 ms
    write8(0xF5, 0b10110000);
    return true;
}

int32_t BMP280::readRawTemp() {
    _wire->beginTransmission(_addr);
    _wire->write(0xFA);                // temp_msb
    _wire->endTransmission();
    _wire->requestFrom(_addr, (uint8_t)3);
    int32_t msb  = _wire->read();
    int32_t lsb  = _wire->read();
    int32_t xlsb = _wire->read();
    return (msb << 12) | (lsb << 4) | (xlsb >> 4);
}

int32_t BMP280::readRawPressure() {
    _wire->beginTransmission(_addr);
    _wire->write(0xF7);                // press_msb
    _wire->endTransmission();
    _wire->requestFrom(_addr, (uint8_t)3);
    int32_t msb  = _wire->read();
    int32_t lsb  = _wire->read();
    int32_t xlsb = _wire->read();
    return (msb << 12) | (lsb << 4) | (xlsb >> 4);
}

float BMP280::readTemperature() {
    int32_t adc_T = readRawTemp();
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)_calib.dig_T1 << 1))) *
                     ((int32_t)_calib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)_calib.dig_T1)) *
                       ((adc_T >> 4) - ((int32_t)_calib.dig_T1))) >> 12) *
                     ((int32_t)_calib.dig_T3)) >> 14;
    _t_fine = var1 + var2;
    return ((_t_fine * 5 + 128) >> 8) / 100.0f;
}

float BMP280::readPressure() {
    // Must call readTemperature() first to populate _t_fine
    int32_t adc_P = readRawPressure();
    int64_t var1 = ((int64_t)_t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)_calib.dig_P2) << 12);
    var1 = (((int64_t)1 << 47) + var1) * ((int64_t)_calib.dig_P1) >> 33;
    if (var1 == 0) return 0.0f;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)_calib.dig_P7) << 4);
    return (float)p / 25600.0f;     // hPa
}
