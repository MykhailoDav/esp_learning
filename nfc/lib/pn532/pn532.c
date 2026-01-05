#include "pn532.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "PN532";

// Допоміжні функції для роботи з I2C
static esp_err_t pn532_i2c_write(pn532_t *pn532, const uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (pn532->i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(pn532->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t pn532_i2c_read(pn532_t *pn532, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (pn532->i2c_address << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(pn532->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Перевірка готовності PN532
static bool pn532_is_ready(pn532_t *pn532) {
    uint8_t status;
    if (pn532_i2c_read(pn532, &status, 1) == ESP_OK) {
        return (status & 0x01) == 0x01;
    }
    return false;
}

// Пробудження PN532 (потрібно для I2C режиму)
static esp_err_t pn532_wakeup(pn532_t *pn532) {
    // Відправляємо SAM Configuration для пробудження
    uint8_t wakeup_cmd[] = {
        PN532_PREAMBLE,
        PN532_STARTCODE1,
        PN532_STARTCODE2,
        0x03,  // Length
        0xFD,  // Length checksum
        PN532_HOSTTOPN532,
        PN532_COMMAND_SAMCONFIGURATION,
        0x01,  // Normal mode
        0x00,  // Checksum
        PN532_POSTAMBLE
    };
    
    // Обчислюємо правильну контрольну суму
    uint8_t checksum = PN532_HOSTTOPN532 + PN532_COMMAND_SAMCONFIGURATION + 0x01;
    wakeup_cmd[8] = ~checksum;
    
    esp_err_t ret = pn532_i2c_write(pn532, wakeup_cmd, sizeof(wakeup_cmd));
    if (ret != ESP_OK) {
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Даємо час на пробудження
    
    return ESP_OK;
}

// Очікування готовності
static esp_err_t pn532_wait_ready(pn532_t *pn532, uint32_t timeout_ms) {
    uint32_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        if (pn532_is_ready(pn532)) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_ERR_TIMEOUT;
}

// Читання ACK
static esp_err_t pn532_read_ack(pn532_t *pn532) {
    uint8_t ack_buf[7];  // +1 для status byte
    const uint8_t pn532_ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    
    vTaskDelay(pdMS_TO_TICKS(10));  // Коротка пауза перед читанням
    
    if (pn532_wait_ready(pn532, PN532_ACK_WAIT_TIME) != ESP_OK) {
        ESP_LOGW(TAG, "ACK timeout - PN532 not ready");
        return ESP_ERR_TIMEOUT;
    }
    
    if (pn532_i2c_read(pn532, ack_buf, 7) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read ACK from I2C");
        return ESP_FAIL;
    }
    
    // Додаємо діагностичне логування
    ESP_LOGD(TAG, "ACK buffer: %02X %02X %02X %02X %02X %02X %02X",
             ack_buf[0], ack_buf[1], ack_buf[2], ack_buf[3], 
             ack_buf[4], ack_buf[5], ack_buf[6]);
    
    if (memcmp(&ack_buf[1], pn532_ack, 6) != 0) {
        ESP_LOGW(TAG, "Invalid ACK frame");
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "ACK received successfully");
    return ESP_OK;
}

// Відправка команди
static esp_err_t pn532_send_command(pn532_t *pn532, const uint8_t *cmd, uint8_t cmd_len) {
    uint8_t checksum;
    uint8_t frame_len = cmd_len + 1; // +1 для коду команди
    
    // Формуємо фрейм
    uint8_t frame[256];
    uint8_t idx = 0;
    
    frame[idx++] = PN532_PREAMBLE;
    frame[idx++] = PN532_STARTCODE1;
    frame[idx++] = PN532_STARTCODE2;
    frame[idx++] = frame_len;
    frame[idx++] = (~frame_len + 1);
    frame[idx++] = PN532_HOSTTOPN532;
    
    // Обчислюємо checksum
    checksum = PN532_HOSTTOPN532;
    
    for (uint8_t i = 0; i < cmd_len; i++) {
        frame[idx++] = cmd[i];
        checksum += cmd[i];
    }
    
    frame[idx++] = ~checksum;
    frame[idx++] = PN532_POSTAMBLE;
    
    return pn532_i2c_write(pn532, frame, idx);
}

// Читання відповіді
static esp_err_t pn532_read_response(pn532_t *pn532, uint8_t *response, uint8_t *response_len, uint32_t timeout_ms) {
    if (pn532_wait_ready(pn532, timeout_ms) != ESP_OK) {
        ESP_LOGW(TAG, "Response timeout");
        return ESP_ERR_TIMEOUT;
    }
    
    uint8_t header[7];
    if (pn532_i2c_read(pn532, header, 7) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // Перевіряємо заголовок
    if (header[1] != PN532_PREAMBLE || header[2] != PN532_STARTCODE1 || 
        header[3] != PN532_STARTCODE2) {
        ESP_LOGW(TAG, "Invalid response header");
        return ESP_FAIL;
    }
    
    uint8_t len = header[4];
    uint8_t len_checksum = header[5];
    
    if ((uint8_t)(len + len_checksum) != 0) {
        ESP_LOGW(TAG, "Invalid length checksum");
        return ESP_FAIL;
    }
    
    if (header[6] != PN532_PN532TOHOST) {
        ESP_LOGW(TAG, "Invalid frame identifier");
        return ESP_FAIL;
    }
    
    len -= 2; // Віднімаємо TFI та command code
    
    // Читаємо дані
    uint8_t data[256];
    if (pn532_i2c_read(pn532, data, len + 2) != ESP_OK) { // +2 для checksum та postamble
        return ESP_FAIL;
    }
    
    // Копіюємо дані в response (без command code)
    memcpy(response, &data[1], len);
    *response_len = len;
    
    return ESP_OK;
}

esp_err_t pn532_init(pn532_t *pn532, i2c_port_t i2c_port, int sda_pin, int scl_pin) {
    ESP_LOGI(TAG, "Initializing PN532 on I2C port %d (SDA: GPIO%d, SCL: GPIO%d)", 
             i2c_port, sda_pin, scl_pin);
    
    pn532->i2c_port = i2c_port;
    pn532->i2c_address = PN532_I2C_ADDRESS;
    
    // Налаштування I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100kHz для надійності
    };
    
    esp_err_t ret = i2c_param_config(i2c_port, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Даємо час модулю на ініціалізацію
    ESP_LOGI(TAG, "Waiting for PN532 to power up...");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Пробуджуємо PN532 (важливо для I2C режиму)
    ESP_LOGI(TAG, "Waking up PN532...");
    ret = pn532_wakeup(pn532);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Wakeup command failed, continuing anyway...");
    }
    
    // Додаткова затримка після пробудження
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Перевіряємо зв'язок - отримуємо версію firmware
    ESP_LOGI(TAG, "Getting firmware version...");
    uint32_t version;
    ret = pn532_get_firmware_version(pn532, &version);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get firmware version");
        ESP_LOGE(TAG, "Make sure PN532 is in I2C mode (check DIP switches)");
        return ret;
    }
    
    ESP_LOGI(TAG, "PN532 Firmware version: %lu.%lu", (version >> 8) & 0xFF, version & 0xFF);
    
    // Налаштовуємо SAM
    ret = pn532_sam_configuration(pn532);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SAM configuration failed");
        return ret;
    }
    
    ESP_LOGI(TAG, "PN532 initialized successfully!");
    return ESP_OK;
}

esp_err_t pn532_get_firmware_version(pn532_t *pn532, uint32_t *version) {
    uint8_t cmd[] = {PN532_COMMAND_GETFIRMWAREVERSION};
    uint8_t response[12];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_response(pn532, response, &response_len, PN532_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return ret;
    }
    
    if (response_len < 4) {
        return ESP_FAIL;
    }
    
    *version = (response[1] << 8) | response[2];
    
    return ESP_OK;
}

esp_err_t pn532_sam_configuration(pn532_t *pn532) {
    uint8_t cmd[] = {PN532_COMMAND_SAMCONFIGURATION, 0x01, 0x14, 0x01};
    uint8_t response[8];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_response(pn532, response, &response_len, PN532_TIMEOUT_MS);
    
    return ret;
}

esp_err_t pn532_read_passive_target(pn532_t *pn532, pn532_card_info_t *card_info, uint32_t timeout_ms) {
    uint8_t cmd[] = {PN532_COMMAND_INLISTPASSIVETARGET, 0x01, 0x00}; // 0x00 = 106 kbps type A (ISO/IEC14443 Type A)
    uint8_t response[64];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = pn532_read_response(pn532, response, &response_len, timeout_ms);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Перевіряємо, чи знайдено мітку
    if (response_len < 1 || response[0] != 0x01) {
        return ESP_ERR_NOT_FOUND;
    }
    
    // Парсимо інформацію про картку
    // response[1] = tag number
    // response[2-3] = SENS_RES (ATQA)
    // response[4] = SEL_RES (SAK)
    // response[5] = UID length
    // response[6...] = UID
    
    if (response_len < 6) {
        return ESP_FAIL;
    }
    
    card_info->atqa = (response[2] << 8) | response[3];
    card_info->sak = response[4];
    card_info->uid_length = response[5];
    
    if (card_info->uid_length > PN532_MAX_UID_LENGTH) {
        ESP_LOGW(TAG, "UID too long: %d", card_info->uid_length);
        return ESP_FAIL;
    }
    
    memcpy(card_info->uid, &response[6], card_info->uid_length);
    
    return ESP_OK;
}

void pn532_print_uid(const uint8_t *uid, uint8_t uid_length) {
    printf("UID:");
    for (uint8_t i = 0; i < uid_length; i++) {
        printf(" %02X", uid[i]);
    }
    printf("\n");
}
