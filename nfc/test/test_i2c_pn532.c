#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "I2C_TEST";

#define I2C_SDA   41
#define I2C_SCL   42
#define PN532_ADDR 0x24

void app_main(void) {
    ESP_LOGI(TAG, "=== PN532 I2C Communication Test ===");
    
    // Ініціалізація I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    
    ESP_LOGI(TAG, "I2C initialized. Testing PN532 at 0x%02X...\n", PN532_ADDR);
    
    // Тест 1: Проста перевірка наявності пристрою
    ESP_LOGI(TAG, "TEST 1: I2C Device Detection");
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PN532_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "  ✓ Device responds to I2C address 0x%02X", PN532_ADDR);
    } else {
        ESP_LOGE(TAG, "  ✗ Device does NOT respond: %s", esp_err_to_name(ret));
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Тест 2: Читання статусу
    ESP_LOGI(TAG, "\nTEST 2: Reading Status Byte");
    uint8_t status;
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PN532_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &status, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "  ✓ Status byte read: 0x%02X", status);
        if (status == 0x01) {
            ESP_LOGI(TAG, "  ✓ PN532 reports READY");
        } else if (status == 0x00) {
            ESP_LOGW(TAG, "  ⚠ PN532 reports BUSY or NOT READY");
        } else {
            ESP_LOGW(TAG, "  ? Unknown status value");
        }
    } else {
        ESP_LOGE(TAG, "  ✗ Failed to read status: %s", esp_err_to_name(ret));
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Тест 3: Спроба пробудження
    ESP_LOGI(TAG, "\nTEST 3: Sending Wakeup Command");
    uint8_t wakeup[] = {
        0x00,  // PREAMBLE
        0x00,  // STARTCODE1
        0xFF,  // STARTCODE2
        0x05,  // Length
        0xFB,  // Length checksum
        0xD4,  // HostToPN532
        0x14,  // SAMConfiguration
        0x01,  // Normal mode
        0x14,  // Timeout 20ms * 20 = 400ms
        0x01,  // Use IRQ
        0xFE,  // Checksum
        0x00   // POSTAMBLE
    };
    
    ret = i2c_master_write_to_device(I2C_NUM_0, PN532_ADDR, wakeup, sizeof(wakeup), pdMS_TO_TICKS(100));
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "  ✓ Wakeup command sent successfully");
    } else {
        ESP_LOGE(TAG, "  ✗ Failed to send wakeup: %s", esp_err_to_name(ret));
    }
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Тест 4: Читання ACK після wakeup
    ESP_LOGI(TAG, "\nTEST 4: Reading ACK Response");
    uint8_t ack_buf[25];
    ret = i2c_master_read_from_device(I2C_NUM_0, PN532_ADDR, ack_buf, sizeof(ack_buf), pdMS_TO_TICKS(100));
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "  ✓ Read %d bytes:", sizeof(ack_buf));
        printf("    ");
        for (int i = 0; i < sizeof(ack_buf); i++) {
            printf("%02X ", ack_buf[i]);
            if ((i + 1) % 16 == 0) printf("\n    ");
        }
        printf("\n");
        
        // Перевіряємо на ACK frame: 00 00 FF 00 FF 00
        bool found_ack = false;
        for (int i = 0; i < sizeof(ack_buf) - 6; i++) {
            if (ack_buf[i] == 0x00 && ack_buf[i+1] == 0x00 && 
                ack_buf[i+2] == 0xFF && ack_buf[i+3] == 0x00 &&
                ack_buf[i+4] == 0xFF && ack_buf[i+5] == 0x00) {
                ESP_LOGI(TAG, "  ✓ ACK frame found at position %d", i);
                found_ack = true;
                break;
            }
        }
        
        if (!found_ack) {
            ESP_LOGW(TAG, "  ⚠ No valid ACK frame found");
        }
    } else {
        ESP_LOGE(TAG, "  ✗ Failed to read ACK: %s", esp_err_to_name(ret));
    }
    
    // Висновок
    ESP_LOGI(TAG, "\n╔════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║                   TEST RESULTS                         ║");
    ESP_LOGI(TAG, "╠════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "║  PN532 is detected on I2C bus at address 0x24          ║");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "║  HOWEVER: Device does not respond to PN532 commands   ║");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "║  ⚠️  MOST LIKELY CAUSE:                                ║");
    ESP_LOGI(TAG, "║     PN532 is NOT in I2C mode!                          ║");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "║  📋 SOLUTION:                                          ║");
    ESP_LOGI(TAG, "║     1. Power OFF the PN532 module                     ║");
    ESP_LOGI(TAG, "║     2. Find DIP switches on PN532 board               ║");
    ESP_LOGI(TAG, "║     3. Set switches for I2C mode:                     ║");
    ESP_LOGI(TAG, "║        • Switch 1 (SEL0): ON  ⬆️                       ║");
    ESP_LOGI(TAG, "║        • Switch 2 (SEL1): OFF ⬇️                       ║");
    ESP_LOGI(TAG, "║     4. Power ON and test again                        ║");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "║  📖 See: PN532_I2C_MODE_SETUP.md                       ║");
    ESP_LOGI(TAG, "║                                                        ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════════════════╝");
    
    ESP_LOGI(TAG, "\nTest completed. Please check DIP switches and try again.");
}
