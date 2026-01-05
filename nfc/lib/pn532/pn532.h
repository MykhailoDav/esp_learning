#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"
#include "esp_err.h"

// I2C адреса PN532
#define PN532_I2C_ADDRESS           0x24

// Команди PN532
#define PN532_COMMAND_GETFIRMWAREVERSION    0x02
#define PN532_COMMAND_SAMCONFIGURATION      0x14
#define PN532_COMMAND_INLISTPASSIVETARGET   0x4A

// Константи
#define PN532_PREAMBLE              0x00
#define PN532_STARTCODE1            0x00
#define PN532_STARTCODE2            0xFF
#define PN532_POSTAMBLE             0x00
#define PN532_HOSTTOPN532           0xD4
#define PN532_PN532TOHOST           0xD5

// Таймаути
#define PN532_ACK_WAIT_TIME         100   // Збільшено з 10 до 100ms
#define PN532_TIMEOUT_MS            2000  // Збільшено з 1000 до 2000ms

// Максимальна довжина UID
#define PN532_MAX_UID_LENGTH        7

// Структура для зберігання інформації про PN532
typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_address;
} pn532_t;

// Структура для зберігання інформації про картку/мітку
typedef struct {
    uint8_t uid[PN532_MAX_UID_LENGTH];
    uint8_t uid_length;
    uint16_t atqa;
    uint8_t sak;
} pn532_card_info_t;

/**
 * @brief Ініціалізація PN532
 * 
 * @param pn532 Вказівник на структуру pn532_t
 * @param i2c_port Порт I2C (I2C_NUM_0 або I2C_NUM_1)
 * @param sda_pin GPIO пін для SDA
 * @param scl_pin GPIO пін для SCL
 * @return esp_err_t ESP_OK при успіху
 */
esp_err_t pn532_init(pn532_t *pn532, i2c_port_t i2c_port, int sda_pin, int scl_pin);

/**
 * @brief Отримати версію firmware PN532
 * 
 * @param pn532 Вказівник на структуру pn532_t
 * @param version Вказівник для збереження версії (4 байти)
 * @return esp_err_t ESP_OK при успіху
 */
esp_err_t pn532_get_firmware_version(pn532_t *pn532, uint32_t *version);

/**
 * @brief Налаштувати SAM (Security Access Module)
 * 
 * @param pn532 Вказівник на структуру pn532_t
 * @return esp_err_t ESP_OK при успіху
 */
esp_err_t pn532_sam_configuration(pn532_t *pn532);

/**
 * @brief Виявити пасивну мітку/картку ISO14443A (Mifare)
 * 
 * @param pn532 Вказівник на структуру pn532_t
 * @param card_info Вказівник для збереження інформації про картку
 * @param timeout_ms Таймаут очікування в мілісекундах
 * @return esp_err_t ESP_OK при успіху
 */
esp_err_t pn532_read_passive_target(pn532_t *pn532, pn532_card_info_t *card_info, uint32_t timeout_ms);

/**
 * @brief Виведення UID у зручному форматі
 * 
 * @param uid Масив з UID
 * @param uid_length Довжина UID
 */
void pn532_print_uid(const uint8_t *uid, uint8_t uid_length);

#endif // PN532_H
