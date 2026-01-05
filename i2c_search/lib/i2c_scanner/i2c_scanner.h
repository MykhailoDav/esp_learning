/**
 * @file i2c_scanner.h
 * @brief I2C Bus Scanner Library for ESP32
 * 
 * This library provides functions to scan the I2C bus and detect connected devices.
 */

#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C Scanner configuration structure
 */
typedef struct {
    i2c_port_t i2c_port;        /*!< I2C port number */
    gpio_num_t sda_io;          /*!< GPIO number for SDA */
    gpio_num_t scl_io;          /*!< GPIO number for SCL */
    uint32_t clk_speed;         /*!< I2C clock frequency in Hz */
    bool enable_pullup;         /*!< Enable internal pull-up resistors */
} i2c_scanner_config_t;

/**
 * @brief Default I2C scanner configuration
 */
#define I2C_SCANNER_CONFIG_DEFAULT() {              \
    .i2c_port = I2C_NUM_0,                          \
    .sda_io = GPIO_NUM_41,                          \
    .scl_io = GPIO_NUM_42,                          \
    .clk_speed = 100000,                            \
    .enable_pullup = true,                          \
}

/**
 * @brief Initialize I2C scanner with custom configuration
 * 
 * @param config Pointer to I2C scanner configuration structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2c_scanner_init(const i2c_scanner_config_t *config);

/**
 * @brief Deinitialize I2C scanner and free resources
 * 
 * @param i2c_port I2C port number to deinitialize
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2c_scanner_deinit(i2c_port_t i2c_port);

/**
 * @brief Check if a device is present at the specified I2C address
 * 
 * @param i2c_port I2C port number
 * @param address 7-bit I2C device address
 * @return ESP_OK if device found, ESP_ERR_NOT_FOUND if not found, other error codes on failure
 */
esp_err_t i2c_scanner_check_device(i2c_port_t i2c_port, uint8_t address);

/**
 * @brief Scan I2C bus and print found devices in table format
 * 
 * @param i2c_port I2C port number
 * @return Number of devices found
 */
uint8_t i2c_scanner_scan_bus(i2c_port_t i2c_port);

/**
 * @brief Scan I2C bus and store found device addresses in array
 * 
 * @param i2c_port I2C port number
 * @param devices Array to store found device addresses
 * @param max_devices Maximum number of devices to store
 * @return Number of devices found
 */
uint8_t i2c_scanner_get_devices(i2c_port_t i2c_port, uint8_t *devices, uint8_t max_devices);

/**
 * @brief Get device name by I2C address (common devices)
 * 
 * @param address 7-bit I2C device address
 * @return Pointer to device name string or "Unknown" if not in database
 */
const char* i2c_scanner_get_device_name(uint8_t address);

#ifdef __cplusplus
}
#endif

#endif // I2C_SCANNER_H
