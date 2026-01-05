/**
 * @file example_advanced.c
 * @brief Розширений приклад використання бібліотеки I2C Scanner
 * 
 * Цей приклад демонструє:
 * - Користувацьку конфігурацію GPIO та швидкості
 * - Перевірку конкретних пристроїв
 * - Отримання списку адрес
 * - Розпізнавання пристроїв
 */

#include <stdio.h>
#include "i2c_scanner.h"
#include "esp_log.h"

static const char *TAG = "EXAMPLE_ADVANCED";

void app_main(void)
{
    ESP_LOGI(TAG, "Advanced I2C Scanner Example");
    
    // Користувацька конфігурація
    i2c_scanner_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io = GPIO_NUM_21,      // Можна змінити на інший GPIO
        .scl_io = GPIO_NUM_22,      // Можна змінити на інший GPIO
        .clk_speed = 100000,        // 100 kHz (Standard Mode)
        .enable_pullup = true,      // Ввімкнути внутрішні pull-up
    };
    
    // Ініціалізація
    esp_err_t ret = i2c_scanner_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C scanner");
        return;
    }
    
    ESP_LOGI(TAG, "Configuration: SDA=GPIO%d, SCL=GPIO%d, Speed=%lu Hz",
             config.sda_io, config.scl_io, config.clk_speed);
    
    // 1. Перевірка конкретного пристрою (наприклад, OLED на 0x3C)
    ESP_LOGI(TAG, "\n=== Checking specific device (0x3C) ===");
    ret = i2c_scanner_check_device(config.i2c_port, 0x3C);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OLED Display (0x3C) is connected!");
    } else {
        ESP_LOGW(TAG, "OLED Display (0x3C) not found");
    }
    
    // 2. Повне сканування шини
    ESP_LOGI(TAG, "\n=== Full bus scan ===");
    printf("\n");
    uint8_t devices_found = i2c_scanner_scan_bus(config.i2c_port);
    
    // 3. Отримання списку знайдених адрес
    if (devices_found > 0) {
        ESP_LOGI(TAG, "\n=== Device list ===");
        uint8_t devices[20];
        uint8_t count = i2c_scanner_get_devices(config.i2c_port, devices, 20);
        
        printf("\nКнайдено %d пристрій(ів):\n", count);
        printf("┌────┬──────────┬──────────────────────────────┐\n");
        printf("│ № │  Адреса  │         Назва                │\n");
        printf("├────┼──────────┼──────────────────────────────┤\n");
        
        for (int i = 0; i < count; i++) {
            const char *name = i2c_scanner_get_device_name(devices[i]);
            printf("│ %2d │   0x%02X   │ %-28s │\n", i + 1, devices[i], name);
        }
        printf("└────┴──────────┴──────────────────────────────┘\n");
    }
    
    // 4. Перевірка декількох популярних адрес
    ESP_LOGI(TAG, "\n=== Checking common devices ===");
    uint8_t common_addresses[] = {0x3C, 0x3D, 0x48, 0x68, 0x76, 0x77};
    int common_count = sizeof(common_addresses) / sizeof(common_addresses[0]);
    
    for (int i = 0; i < common_count; i++) {
        uint8_t addr = common_addresses[i];
        ret = i2c_scanner_check_device(config.i2c_port, addr);
        const char *name = i2c_scanner_get_device_name(addr);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ 0x%02X: %s - FOUND", addr, name);
        } else {
            ESP_LOGD(TAG, "✗ 0x%02X: %s - not found", addr, name);
        }
    }
    
    ESP_LOGI(TAG, "\nExample complete!");
}
