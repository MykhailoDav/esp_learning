#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pn532_uart.h"

static const char *TAG = "NFC_UART";

// UART pins для PN532
#define PN532_TX_PIN 41 // ESP32 TX -> PN532 RX
#define PN532_RX_PIN 42 // ESP32 RX -> PN532 TX
#define PN532_UART_PORT UART_NUM_1

// Helper function to get card type
static const char *get_card_type(uint16_t atqa, uint8_t sak)
{
    if (sak == 0x08)
        return "Mifare Classic 1K";
    if (sak == 0x18)
        return "Mifare Classic 4K";
    if (sak == 0x00)
        return "Mifare Ultralight/NTAG";
    if (sak == 0x20)
        return "Mifare Plus/DESFire";
    return "Unknown";
}

void app_main(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║    PN532 NFC Reader - UART Mode            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "UART Configuration:");
    ESP_LOGI(TAG, "  Port: UART%d", PN532_UART_PORT);
    ESP_LOGI(TAG, "  TX (ESP32 -> PN532): GPIO %d", PN532_TX_PIN);
    ESP_LOGI(TAG, "  RX (ESP32 <- PN532): GPIO %d", PN532_RX_PIN);
    ESP_LOGI(TAG, "  Baud Rate: 115200");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "⚠️  PN532 Mode Settings (DIP switches):");
    ESP_LOGI(TAG, "  Switch 1: ON  ⬆️");
    ESP_LOGI(TAG, "  Switch 2: ON  ⬆️  (UART/HSU mode)");
    ESP_LOGI(TAG, "");

    // Initialize PN532
    pn532_uart_t pn532;
    esp_err_t ret = pn532_uart_init(&pn532, PN532_UART_PORT, PN532_TX_PIN, PN532_RX_PIN);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "╔════════════════════════════════════════════╗");
        ESP_LOGE(TAG, "║  Failed to initialize PN532!               ║");
        ESP_LOGE(TAG, "╠════════════════════════════════════════════╣");
        ESP_LOGE(TAG, "║  Check:                                    ║");
        ESP_LOGE(TAG, "║  1. DIP Switches: SW1=ON, SW2=ON           ║");
        ESP_LOGE(TAG, "║  2. Connections:                           ║");
        ESP_LOGE(TAG, "║     ESP32 GPIO%d -> PN532 RX               ║", PN532_TX_PIN);
        ESP_LOGE(TAG, "║     ESP32 GPIO%d -> PN532 TX               ║", PN532_RX_PIN);
        ESP_LOGE(TAG, "║     VCC -> 3.3V, GND -> GND                ║");
        ESP_LOGE(TAG, "╚════════════════════════════════════════════╝");
        return;
    }

    ESP_LOGI(TAG, "✅ PN532 ready!");
    ESP_LOGI(TAG, "📱 Place a card/tag on the reader...");
    ESP_LOGI(TAG, "");

    pn532_card_t card;
    bool card_was_present = false;

    // Main loop
    while (1)
    {
        ret = pn532_uart_read_passive_target(&pn532, &card, 500);

        if (ret == ESP_OK)
        {
            if (!card_was_present)
            {
                // Card detected
                ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
                ESP_LOGI(TAG, "║          🎉 CARD DETECTED!                 ║");
                ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");

                // Print UID
                printf("  UID (%d bytes): ", card.uid_length);
                for (int i = 0; i < card.uid_length; i++)
                {
                    printf("%02X", card.uid[i]);
                    if (i < card.uid_length - 1)
                        printf(" ");
                }
                printf("\n");

                // Print card info
                printf("  ATQA: 0x%04X\n", card.atqa);
                printf("  SAK:  0x%02X\n", card.sak);
                printf("  Type: %s\n", get_card_type(card.atqa, card.sak));

                ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
                printf("\n");

                card_was_present = true;
            }
        }
        else
        {
            if (card_was_present)
            {
                ESP_LOGI(TAG, "📤 Card removed\n");
                card_was_present = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
