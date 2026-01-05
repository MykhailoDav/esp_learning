/**
 * @file pn532_uart.h
 * @brief PN532 NFC/RFID Reader Library for UART (HSU mode)
 * 
 * Простіша та надійніша альтернатива I2C режиму
 */

#ifndef PN532_UART_H
#define PN532_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_err.h"

// UART Configuration
#define PN532_UART_BAUD_RATE    115200
#define PN532_UART_BUF_SIZE     1024

// PN532 Commands
#define PN532_CMD_GETFIRMWAREVERSION    0x02
#define PN532_CMD_SAMCONFIGURATION      0x14
#define PN532_CMD_INLISTPASSIVETARGET   0x4A

// Frame constants
#define PN532_PREAMBLE          0x00
#define PN532_STARTCODE1        0x00
#define PN532_STARTCODE2        0xFF
#define PN532_POSTAMBLE         0x00
#define PN532_HOSTTOPN532       0xD4
#define PN532_PN532TOHOST       0xD5

// Timeouts
#define PN532_TIMEOUT_MS        1000

// Max UID length
#define PN532_MAX_UID_LENGTH    7

// PN532 UART instance
typedef struct {
    uart_port_t uart_port;
    int tx_pin;
    int rx_pin;
} pn532_uart_t;

// Card information
typedef struct {
    uint8_t uid[PN532_MAX_UID_LENGTH];
    uint8_t uid_length;
    uint16_t atqa;
    uint8_t sak;
} pn532_card_t;

/**
 * @brief Initialize PN532 in UART mode
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @param uart_port UART port (UART_NUM_0, UART_NUM_1, UART_NUM_2)
 * @param tx_pin TX GPIO pin
 * @param rx_pin RX GPIO pin
 * @return ESP_OK on success
 */
esp_err_t pn532_uart_init(pn532_uart_t *pn532, uart_port_t uart_port, int tx_pin, int rx_pin);

/**
 * @brief Get PN532 firmware version
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @param version Pointer to store version (major.minor)
 * @return ESP_OK on success
 */
esp_err_t pn532_uart_get_firmware_version(pn532_uart_t *pn532, uint32_t *version);

/**
 * @brief Configure SAM (Secure Access Module)
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @return ESP_OK on success
 */
esp_err_t pn532_uart_sam_config(pn532_uart_t *pn532);

/**
 * @brief Read passive target (card/tag)
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @param card Pointer to pn532_card_t to store card info
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK if card found, ESP_ERR_NOT_FOUND if no card
 */
esp_err_t pn532_uart_read_passive_target(pn532_uart_t *pn532, pn532_card_t *card, uint32_t timeout_ms);

/**
 * @brief Wakeup PN532 from power down mode
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @return ESP_OK on success
 */
esp_err_t pn532_uart_wakeup(pn532_uart_t *pn532);

/**
 * @brief Deinitialize PN532 UART
 * 
 * @param pn532 Pointer to pn532_uart_t structure
 * @return ESP_OK on success
 */
esp_err_t pn532_uart_deinit(pn532_uart_t *pn532);

#endif // PN532_UART_H
