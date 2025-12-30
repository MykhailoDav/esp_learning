#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Scan I2C bus for connected devices
 * 
 * @param i2c_bus I2C master bus handle
 * @param lprint Callback function for printing output (e.g., printf)
 */
void i2c_scan_devices(i2c_master_bus_handle_t i2c_bus, void (*lprint)(const char *));

/**
 * @brief Scan I2C bus and return array of found addresses
 * 
 * @param i2c_bus I2C master bus handle
 * @param addresses Array to store found addresses (must be at least 128 bytes)
 * @param max_devices Maximum number of devices to find
 * @return Number of devices found
 */
uint8_t i2c_scan_get_addresses(i2c_master_bus_handle_t i2c_bus, uint8_t *addresses, uint8_t max_devices);

#ifdef __cplusplus
}
#endif

#endif // I2C_SCANNER_H
