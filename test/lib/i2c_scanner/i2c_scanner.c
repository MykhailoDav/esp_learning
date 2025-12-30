#include "i2c_scanner.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_SCANNER_TIMEOUT_MS 50
#define I2C_ACK 0
#define I2C_NACK 1

static const char *TAG = "I2C_SCANNER";

void i2c_scan_devices(i2c_master_bus_handle_t i2c_bus, void (*lprint)(const char *))
{
    char buf[64];
    esp_err_t res;
    uint8_t found_count = 0;

    lprint("\n     ");
    for(int i = 0; i < 16; i++)
    {
        sprintf(buf, "0%X|", i);
        lprint(buf);
    }

    lprint("\n    ------------------------------------------------");
    lprint("\n 00|                        ");
    
    for(int i = 8; i < 240; i++)
    {
        if(i % 16 == 0) {
            sprintf(buf, "\n %X0|", i/16);
            lprint(buf);
        }

        // Skip odd addresses (only test write addresses)
        if((i & 1) == 0) {
            uint8_t addr = i >> 1;  // Convert to 7-bit address
            
            res = i2c_master_probe(i2c_bus, addr, I2C_SCANNER_TIMEOUT_MS);
            
            if(res == ESP_OK) {
                sprintf(buf, " %02X", addr);
                lprint(buf);
                found_count++;
                ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
            } else {
                lprint(" --");
            }
        } else {
            lprint(" --");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2));  // Small delay between probes
    }
    
    sprintf(buf, "\n\nFound %d device(s)\n", found_count);
    lprint(buf);
}

uint8_t i2c_scan_get_addresses(i2c_master_bus_handle_t i2c_bus, uint8_t *addresses, uint8_t max_devices)
{
    esp_err_t res;
    uint8_t count = 0;

    if(addresses == NULL || max_devices == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return 0;
    }

    ESP_LOGI(TAG, "Scanning I2C bus...");

    for(uint8_t addr = 0x03; addr < 0x78; addr++)
    {
        res = i2c_master_probe(i2c_bus, addr, I2C_SCANNER_TIMEOUT_MS);
        
        if(res == ESP_OK) {
            addresses[count] = addr;
            count++;
            ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
            
            if(count >= max_devices) {
                ESP_LOGW(TAG, "Reached maximum device limit");
                break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2));  // Small delay between probes
    }

    ESP_LOGI(TAG, "Scan complete. Found %d device(s)", count);
    return count;
}
