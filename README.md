# ESP32 Learning Repository

A learning repository for the **Embedded Systems** university course.  
Contains lab assignments and experiments using the **4D Systems ESP32-S3** board.

## Board
**4D Systems gen4-ESP32-S3-R8N16**
- MCU: ESP32-S3
- RAM: 8MB PSRAM
- Flash: 16MB
- Framework: Arduino (PlatformIO)

## Repository Structure

| Folder | Description |
|---|---|
| `lab_1/` | Basics: Serial output, WS2812 RGB LED (NeoPixel) |
| `lab_2/lab_2.1/` | ADC & DAC: photoresistor reading + LED brightness control via PWM |
| `i2c_search/` | I2C bus device scanner |
| `nfc/` | NFC module PN532 integration |
| `test/` | Tests: I2C scanner + SSD1306 OLED display |
| `TFT_LCD/` | TFT LCD display driver |

## Stack
- **PlatformIO** — build system & firmware upload
- **Arduino Framework** — main framework
- **ESP-IDF** — low-level drivers (I2C, LEDC, ADC)
