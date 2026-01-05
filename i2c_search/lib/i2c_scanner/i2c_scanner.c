/**
 * @file i2c_scanner.c
 * @brief I2C Bus Scanner Library Implementation
 */

#include "i2c_scanner.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "I2C_SCANNER";

#define I2C_TIMEOUT_MS      1000
#define I2C_START_ADDR      0x08
#define I2C_END_ADDR        0x77

/**
 * @brief Known I2C device database
 */
typedef struct {
    uint8_t address;
    const char *name;
} i2c_device_info_t;

static const i2c_device_info_t known_devices[] = {
    {0x1D, "ADXL345 Accelerometer"},
    {0x1E, "HMC5883L Compass"},
    {0x20, "PCF8574 I/O Expander"},
    {0x21, "PCF8574 I/O Expander"},
    {0x22, "PCF8574 I/O Expander"},
    {0x23, "BH1750 Light Sensor"},
    {0x24, "PN532 NFC/RFID"},
    {0x27, "PCF8574 LCD Backpack"},
    {0x38, "PCF8574A I/O Expander"},
    {0x39, "TSL2561 Light Sensor"},
    {0x3C, "SSD1306 OLED Display"},
    {0x3D, "SSD1306 OLED Display"},
    {0x40, "PCA9685 PWM Driver"},
    {0x48, "ADS1115 ADC / PCF8591"},
    {0x49, "ADS1115 ADC"},
    {0x4A, "ADS1115 ADC"},
    {0x4B, "ADS1115 ADC"},
    {0x50, "AT24C32 EEPROM"},
    {0x51, "AT24C32 EEPROM"},
    {0x52, "AT24C32 EEPROM"},
    {0x53, "ADXL345 Accelerometer"},
    {0x57, "AT24C32 EEPROM"},
    {0x68, "DS3231 RTC / MPU6050"},
    {0x69, "MPU6050 / MPU9250"},
    {0x76, "BMP280 / BME280"},
    {0x77, "BMP280 / BME280 / BMP180"},
};

esp_err_t i2c_scanner_init(const i2c_scanner_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_io,
        .scl_io_num = config->scl_io,
        .sda_pullup_en = config->enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .scl_pullup_en = config->enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .master.clk_speed = config->clk_speed,
    };

    esp_err_t err = i2c_param_config(config->i2c_port, &i2c_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(config->i2c_port, i2c_conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C scanner initialized on port %d (SDA: GPIO%d, SCL: GPIO%d, Speed: %lu Hz)",
             config->i2c_port, config->sda_io, config->scl_io, config->clk_speed);

    return ESP_OK;
}

esp_err_t i2c_scanner_deinit(i2c_port_t i2c_port)
{
    return i2c_driver_delete(i2c_port);
}

esp_err_t i2c_scanner_check_device(i2c_port_t i2c_port, uint8_t address)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

const char* i2c_scanner_get_device_name(uint8_t address)
{
    for (int i = 0; i < sizeof(known_devices) / sizeof(known_devices[0]); i++) {
        if (known_devices[i].address == address) {
            return known_devices[i].name;
        }
    }
    return "Unknown Device";
}

uint8_t i2c_scanner_scan_bus(i2c_port_t i2c_port)
{
    printf("\n     ");
    for (int i = 0; i < 16; i++) {
        printf("0%X|", i);
    }
    
    printf("\n    ------------------------------------------------");
    printf("\n 00|                        ");
    
    uint8_t devices_found = 0;
    
    for (int i = I2C_START_ADDR; i <= I2C_END_ADDR; i++) {
        if (i % 16 == 0) {
            printf("\n %X0|", i / 16);
        }
        
        esp_err_t ret = i2c_scanner_check_device(i2c_port, i);
        
        if (ret == ESP_OK) {
            printf(" %02X", i);
            devices_found++;
            
            const char *device_name = i2c_scanner_get_device_name(i);
            ESP_LOGI(TAG, "Device found at 0x%02X: %s", i, device_name);
        } else {
            printf(" --");
        }
    }
    
    // Fill remaining cells in last row
    int remaining = (I2C_END_ADDR + 1) % 16;
    if (remaining > 0) {
        for (int i = remaining; i < 16; i++) {
            printf(" --");
        }
    }
    
    printf("\n\n");
    ESP_LOGI(TAG, "Scan complete. Found %d device(s).", devices_found);
    
    return devices_found;
}

uint8_t i2c_scanner_get_devices(i2c_port_t i2c_port, uint8_t *devices, uint8_t max_devices)
{
    if (devices == NULL || max_devices == 0) {
        ESP_LOGE(TAG, "Invalid parameters for get_devices");
        return 0;
    }

    uint8_t count = 0;
    
    for (uint8_t addr = I2C_START_ADDR; addr <= I2C_END_ADDR && count < max_devices; addr++) {
        esp_err_t ret = i2c_scanner_check_device(i2c_port, addr);
        
        if (ret == ESP_OK) {
            devices[count] = addr;
            count++;
            ESP_LOGD(TAG, "Device found at 0x%02X", addr);
        }
    }
    
    return count;
}
