/**
 * @file pn532_uart.c
 * @brief PN532 UART Implementation
 */

#include "pn532_uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "PN532_UART";

// Helper: Calculate checksum
static uint8_t pn532_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ~sum + 1;
}

// Helper: Send command frame
static esp_err_t pn532_send_command(pn532_uart_t *pn532, const uint8_t *cmd, uint8_t cmd_len) {
    uint8_t frame[256];
    uint8_t idx = 0;
    
    // Build frame: PREAMBLE + STARTCODE + LEN + LCS + TFI + DATA + DCS + POSTAMBLE
    frame[idx++] = PN532_PREAMBLE;
    frame[idx++] = PN532_STARTCODE1;
    frame[idx++] = PN532_STARTCODE2;
    
    uint8_t len = cmd_len + 1; // +1 for TFI
    frame[idx++] = len;
    frame[idx++] = ~len + 1; // Length checksum
    
    frame[idx++] = PN532_HOSTTOPN532;
    
    uint8_t checksum = PN532_HOSTTOPN532;
    for (uint8_t i = 0; i < cmd_len; i++) {
        frame[idx++] = cmd[i];
        checksum += cmd[i];
    }
    
    frame[idx++] = ~checksum + 1; // Data checksum
    frame[idx++] = PN532_POSTAMBLE;
    
    // Send frame
    int written = uart_write_bytes(pn532->uart_port, frame, idx);
    if (written != idx) {
        ESP_LOGE(TAG, "Failed to write UART");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Helper: Read ACK
static esp_err_t pn532_read_ack(pn532_uart_t *pn532) {
    const uint8_t ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    uint8_t buf[6];
    
    vTaskDelay(pdMS_TO_TICKS(10));
    
    int len = uart_read_bytes(pn532->uart_port, buf, sizeof(buf), pdMS_TO_TICKS(100));
    if (len != sizeof(ack)) {
        ESP_LOGW(TAG, "ACK: Expected 6 bytes, got %d", len);
        return ESP_FAIL;
    }
    
    if (memcmp(buf, ack, sizeof(ack)) != 0) {
        ESP_LOGW(TAG, "Invalid ACK frame");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, len, ESP_LOG_WARN);
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "ACK received");
    return ESP_OK;
}

// Helper: Read response
static esp_err_t pn532_read_response(pn532_uart_t *pn532, uint8_t *response, uint8_t *response_len, uint32_t timeout_ms) {
    uint8_t header[6];
    
    // Read header: PREAMBLE + STARTCODE + LEN + LCS + TFI
    int len = uart_read_bytes(pn532->uart_port, header, sizeof(header), pdMS_TO_TICKS(timeout_ms));
    if (len != sizeof(header)) {
        ESP_LOGW(TAG, "Response header timeout");
        return ESP_ERR_TIMEOUT;
    }
    
    // Validate header
    if (header[0] != PN532_PREAMBLE || header[1] != PN532_STARTCODE1 || header[2] != PN532_STARTCODE2) {
        ESP_LOGW(TAG, "Invalid response header");
        return ESP_FAIL;
    }
    
    uint8_t data_len = header[3];
    uint8_t len_checksum = header[4];
    
    if ((uint8_t)(data_len + len_checksum) != 0) {
        ESP_LOGW(TAG, "Invalid length checksum");
        return ESP_FAIL;
    }
    
    if (header[5] != PN532_PN532TOHOST) {
        ESP_LOGW(TAG, "Invalid TFI");
        return ESP_FAIL;
    }
    
    // Read data + checksum + postamble
    uint8_t payload_len = data_len - 1; // -1 for TFI
    uint8_t *payload = malloc(payload_len + 2); // +2 for DCS and POSTAMBLE
    if (!payload) {
        return ESP_ERR_NO_MEM;
    }
    
    len = uart_read_bytes(pn532->uart_port, payload, payload_len + 2, pdMS_TO_TICKS(timeout_ms));
    if (len != payload_len + 2) {
        free(payload);
        ESP_LOGW(TAG, "Response data timeout");
        return ESP_ERR_TIMEOUT;
    }
    
    // Verify data checksum
    uint8_t checksum = PN532_PN532TOHOST;
    for (int i = 0; i < payload_len; i++) {
        checksum += payload[i];
    }
    checksum = ~checksum + 1;
    
    if (payload[payload_len] != checksum) {
        free(payload);
        ESP_LOGW(TAG, "Invalid data checksum");
        return ESP_FAIL;
    }
    
    // Copy response (skip command echo)
    memcpy(response, &payload[1], payload_len - 1);
    *response_len = payload_len - 1;
    
    free(payload);
    return ESP_OK;
}

esp_err_t pn532_uart_wakeup(pn532_uart_t *pn532) {
    // Send wakeup preamble (55 55 00 00 00...)
    uint8_t wakeup[16];
    memset(wakeup, 0x55, 10);
    memset(&wakeup[10], 0x00, 6);
    
    uart_write_bytes(pn532->uart_port, wakeup, sizeof(wakeup));
    uart_flush(pn532->uart_port);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Clear RX buffer
    uart_flush_input(pn532->uart_port);
    
    return ESP_OK;
}

esp_err_t pn532_uart_init(pn532_uart_t *pn532, uart_port_t uart_port, int tx_pin, int rx_pin) {
    ESP_LOGI(TAG, "Initializing PN532 UART on port %d (TX: GPIO%d, RX: GPIO%d)", uart_port, tx_pin, rx_pin);
    
    pn532->uart_port = uart_port;
    pn532->tx_pin = tx_pin;
    pn532->rx_pin = rx_pin;
    
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = PN532_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_param_config(uart_port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_port, PN532_UART_BUF_SIZE, PN532_UART_BUF_SIZE, 0, NULL, 0));
    
    // Wakeup PN532
    ESP_LOGI(TAG, "Waking up PN532...");
    pn532_uart_wakeup(pn532);
    
    // Get firmware version
    uint32_t version;
    esp_err_t ret = pn532_uart_get_firmware_version(pn532, &version);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get firmware version");
        ESP_LOGE(TAG, "Make sure PN532 is in UART/HSU mode:");
        ESP_LOGE(TAG, "  Switch 1: ON");
        ESP_LOGE(TAG, "  Switch 2: ON");
        return ret;
    }
    
    ESP_LOGI(TAG, "PN532 Firmware: v%lu.%lu", (version >> 8) & 0xFF, version & 0xFF);
    
    // Configure SAM
    ret = pn532_uart_sam_config(pn532);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SAM configuration failed");
        return ret;
    }
    
    ESP_LOGI(TAG, "PN532 UART initialized successfully!");
    return ESP_OK;
}

esp_err_t pn532_uart_get_firmware_version(pn532_uart_t *pn532, uint32_t *version) {
    uint8_t cmd[] = {PN532_CMD_GETFIRMWAREVERSION};
    uint8_t response[12];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_response(pn532, response, &response_len, PN532_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;
    
    if (response_len < 4) {
        ESP_LOGE(TAG, "Invalid version response");
        return ESP_FAIL;
    }
    
    *version = (response[1] << 8) | response[2];
    return ESP_OK;
}

esp_err_t pn532_uart_sam_config(pn532_uart_t *pn532) {
    uint8_t cmd[] = {PN532_CMD_SAMCONFIGURATION, 0x01, 0x14, 0x01};
    uint8_t response[8];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_response(pn532, response, &response_len, PN532_TIMEOUT_MS);
    return ret;
}

esp_err_t pn532_uart_read_passive_target(pn532_uart_t *pn532, pn532_card_t *card, uint32_t timeout_ms) {
    uint8_t cmd[] = {PN532_CMD_INLISTPASSIVETARGET, 0x01, 0x00}; // Max 1 card, 106 kbps type A
    uint8_t response[64];
    uint8_t response_len;
    
    esp_err_t ret = pn532_send_command(pn532, cmd, sizeof(cmd));
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_ack(pn532);
    if (ret != ESP_OK) return ret;
    
    ret = pn532_read_response(pn532, response, &response_len, timeout_ms);
    if (ret != ESP_OK) return ret;
    
    if (response_len < 7) {
        return ESP_ERR_NOT_FOUND;
    }
    
    uint8_t num_targets = response[0];
    if (num_targets != 1) {
        return ESP_ERR_NOT_FOUND;
    }
    
    // Parse card info
    card->atqa = (response[3] << 8) | response[2];
    card->sak = response[4];
    card->uid_length = response[5];
    
    if (card->uid_length > PN532_MAX_UID_LENGTH) {
        card->uid_length = PN532_MAX_UID_LENGTH;
    }
    
    memcpy(card->uid, &response[6], card->uid_length);
    
    return ESP_OK;
}

esp_err_t pn532_uart_deinit(pn532_uart_t *pn532) {
    return uart_driver_delete(pn532->uart_port);
}
