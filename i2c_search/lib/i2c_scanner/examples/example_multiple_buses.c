/**
 * @file example_multiple_buses.c
 * @brief Приклад роботи з декількома I2C шинами
 * 
 * ESP32 має два I2C контролери (I2C_NUM_0 та I2C_NUM_1)
 * Цей приклад показує як сканувати обидві шини
 */

#include <stdio.h>
#include "i2c_scanner.h"
#include "esp_log.h"

static const char *TAG = "EXAMPLE_MULTI_BUS";

void app_main(void)
{
    ESP_LOGI(TAG, "Multiple I2C Bus Scanner Example");
    
    // Конфігурація для першої I2C шини
    i2c_scanner_config_t config_bus0 = {
        .i2c_port = I2C_NUM_0,
        .sda_io = GPIO_NUM_21,
        .scl_io = GPIO_NUM_22,
        .clk_speed = 100000,
        .enable_pullup = true,
    };
    
    // Конфігурація для другої I2C шини
    i2c_scanner_config_t config_bus1 = {
        .i2c_port = I2C_NUM_1,
        .sda_io = GPIO_NUM_18,
        .scl_io = GPIO_NUM_19,
        .clk_speed = 100000,
        .enable_pullup = true,
    };
    
    // Ініціалізація обох шин
    ESP_LOGI(TAG, "Initializing I2C Bus 0...");
    esp_err_t ret = i2c_scanner_init(&config_bus0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C Bus 0");
        return;
    }
    
    ESP_LOGI(TAG, "Initializing I2C Bus 1...");
    ret = i2c_scanner_init(&config_bus1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C Bus 1");
        return;
    }
    
    // Сканування першої шини
    ESP_LOGI(TAG, "\n=== Scanning I2C Bus 0 (SDA:GPIO%d, SCL:GPIO%d) ===",
             config_bus0.sda_io, config_bus0.scl_io);
    printf("\n");
    uint8_t count_bus0 = i2c_scanner_scan_bus(I2C_NUM_0);
    ESP_LOGI(TAG, "Bus 0: Found %d device(s)\n", count_bus0);
    
    // Сканування другої шини
    ESP_LOGI(TAG, "\n=== Scanning I2C Bus 1 (SDA:GPIO%d, SCL:GPIO%d) ===",
             config_bus1.sda_io, config_bus1.scl_io);
    printf("\n");
    uint8_t count_bus1 = i2c_scanner_scan_bus(I2C_NUM_1);
    ESP_LOGI(TAG, "Bus 1: Found %d device(s)\n", count_bus1);
    
    // Підсумок
    ESP_LOGI(TAG, "\n=== Summary ===");
    ESP_LOGI(TAG, "Total devices found: %d", count_bus0 + count_bus1);
    ESP_LOGI(TAG, "  - Bus 0: %d devices", count_bus0);
    ESP_LOGI(TAG, "  - Bus 1: %d devices", count_bus1);
    
    // Деініціалізація (якщо потрібно)
    // i2c_scanner_deinit(I2C_NUM_0);
    // i2c_scanner_deinit(I2C_NUM_1);
}
