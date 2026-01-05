/**
 * @file example_basic.c
 * @brief Базовий приклад використання бібліотеки I2C Scanner
 * 
 * Цей приклад показує як:
 * - Ініціалізувати I2C сканер
 * - Виконати базове сканування шини
 * - Отримати список знайдених пристроїв
 */

#include <stdio.h>
#include "i2c_scanner.h"
#include "esp_log.h"

static const char *TAG = "EXAMPLE_BASIC";

void app_main(void)
{
    ESP_LOGI(TAG, "Basic I2C Scanner Example");
    
    // Створення конфігурації за замовчуванням
    i2c_scanner_config_t config = I2C_SCANNER_CONFIG_DEFAULT();
    
    // Ініціалізація сканера
    esp_err_t ret = i2c_scanner_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialization failed!");
        return;
    }
    
    ESP_LOGI(TAG, "I2C Scanner initialized on GPIO%d (SDA) and GPIO%d (SCL)", 
             config.sda_io, config.scl_io);
    
    // Сканування шини та виведення таблиці
    printf("\n");
    uint8_t count = i2c_scanner_scan_bus(config.i2c_port);
    
    ESP_LOGI(TAG, "Found %d device(s)", count);
}
