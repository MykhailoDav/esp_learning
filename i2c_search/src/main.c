#include <stdio.h>
#include "i2c_scanner.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== I2C Scanner Application ===");

    // Конфігурація I2C за замовчуванням (GPIO 41 - SDA, GPIO 42 - SCL, 100kHz)
    i2c_scanner_config_t config = I2C_SCANNER_CONFIG_DEFAULT();
    
    // Або можна налаштувати власні параметри:
    // config.sda_io = GPIO_NUM_18;
    // config.scl_io = GPIO_NUM_19;
    // config.clk_speed = 400000; // 400 kHz
    
    // Ініціалізація I2C сканера
    esp_err_t ret = i2c_scanner_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C scanner: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "I2C Scanner initialized successfully");
    ESP_LOGI(TAG, "Starting I2C bus scan...\n");
    
    // Сканування I2C шини
    uint8_t devices_found = i2c_scanner_scan_bus(config.i2c_port);
    
    // Додатковий приклад: отримання списку пристроїв
    if (devices_found > 0) {
        uint8_t devices[10];
        uint8_t count = i2c_scanner_get_devices(config.i2c_port, devices, 10);
        
        printf("\n=== Знайдені пристрої ===\n");
        for (int i = 0; i < count; i++) {
            const char *name = i2c_scanner_get_device_name(devices[i]);
            printf("  [%d] Address: 0x%02X - %s\n", i + 1, devices[i], name);
        }
        printf("\n");
    }
    
    ESP_LOGI(TAG, "Scan complete. Total devices found: %d", devices_found);
}