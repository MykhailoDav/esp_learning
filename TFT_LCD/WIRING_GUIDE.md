# 📺 Інструкція з підключення 2.8" ILI9341 TFT LCD до ESP32-S3

## 🔌 Апаратне підключення

### Плата: 4D Systems ESP32-S3 GEN4 R8N16

### LCD Модуль: 2.8" SPI TFT ILI9341 240x320 з XPT2046 Touch

---

## 📋 Таблиця підключення

| LCD Pin | Функція | ESP32-S3 GPIO | Опис |
|---------|---------|---------------|------|
| **VCC** | Живлення | **3.3V** | Живлення модуля (НЕ 5V!) |
| **GND** | Земля | **GND** | Загальна земля |
| **CS** | LCD Chip Select | **GPIO 10** | Вибір LCD (активний LOW) |
| **RESET** | Reset | **GPIO 8** | Скидання LCD |
| **DC/RS** | Data/Command | **GPIO 9** | Режим даних/команд |
| **SDI/MOSI** | SPI Data Out | **GPIO 11** | Дані ESP32 → LCD |
| **SCK** | SPI Clock | **GPIO 12** | Синхронізація SPI |
| **LED/BL** | Backlight | **GPIO 14** | Підсвітка (3.3V = ON) |
| **SDO/MISO** | SPI Data In | **GPIO 13** | Дані LCD → ESP32 (для touch) |
| **T_CS** | Touch CS | **GPIO 21** | Вибір touch контролера |
| **T_IRQ** | Touch Interrupt | **GPIO 47** | Сигнал дотику (активний LOW) |

---

## ⚡ Важливі примітки

### ⚠️ Живлення
- **ТІЛЬКИ 3.3V!** ESP32-S3 не толерантний до 5V
- Підключайте VCC до 3.3V пін плати
- Переконайтесь що GND з'єднано

### 🔧 SPI Конфігурація
- **SPI Bus**: SPI2_HOST (HSPI)
- **LCD Clock**: 40 MHz (високошвидкісний)
- **Touch Clock**: 1 MHz (повільніший для стабільності)
- **SPI Mode**: 0 (CPOL=0, CPHA=0)

### 📍 GPIO Обмеження ESP32-S3
✅ **Безпечні GPIO** (можна використовувати):
- GPIO 1-21 (окрім 19, 20 - USB)
- GPIO 33-48

❌ **Уникайте**:
- GPIO 0 (BOOT button)
- GPIO 19, 20 (USB D-, D+)
- GPIO 26-32 (не існують на S3)

---

## 🛠️ Налаштування проекту

### Platform.ini
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
```

### Драйвер LCD
- **ILI9341** драйвер видалено з ESP-IDF 5.5.0
- Використовуйте **ST7789** - повністю сумісний!
- `esp_lcd_new_panel_st7789()` працює ідеально

### Формат кольорів
```c
esp_lcd_panel_dev_config_t cfg = {
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // RGB (не BGR!)
    .bits_per_pixel = 16,  // RGB565
};
esp_lcd_panel_invert_color(panel, false);  // Без інверсії
```

---

## 🎨 Формат RGB565

Кольори кодуються у 16 біт:
```
RRRR RGGG GGGB BBBB
```

### Основні кольори:
- **Червоний**: `0xF800` (11111 000000 00000)
- **Зелений**: `0x07E0` (00000 111111 00000)
- **Синій**: `0x001F` (00000 000000 11111)
- **Білий**: `0xFFFF`
- **Чорний**: `0x0000`
- **Жовтий**: `0xFFE0` (червоний + зелений)

### Калькулятор RGB565:
```c
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
```

---

## 🖼️ Швидкий старт

### 1. Мінімальний код (тест кольорів)
```c
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

void app_main(void) {
    // Ініціалізація SPI bus
    spi_bus_config_t bus = {
        .mosi_io_num = 11,
        .sclk_io_num = 12,
        .max_transfer_sz = 240 * 320 * 2
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    
    // LCD IO config
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = 10,
        .dc_gpio_num = 9,
        .pclk_hz = 40000000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io);
    
    // LCD panel config
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_dev_config_t cfg = {
        .reset_gpio_num = 8,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    esp_lcd_new_panel_st7789(io, &cfg, &panel);
    
    // Ініціалізація
    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);
    esp_lcd_panel_disp_on_off(panel, true);
    
    // Backlight ON
    gpio_set_direction(14, GPIO_MODE_OUTPUT);
    gpio_set_level(14, 1);
    
    // Тест: червоний екран
    uint16_t *buffer = malloc(240 * 320 * 2);
    for(int i = 0; i < 240 * 320; i++) {
        buffer[i] = 0xF800;  // Червоний
    }
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 240, 320, buffer);
}
```

---

## 🎮 Підтримка Touch (XPT2046)

### Ініціалізація Touch
```c
spi_device_handle_t touch_spi;
spi_device_interface_config_t touch_cfg = {
    .clock_speed_hz = 1000000,  // 1 MHz
    .mode = 0,
    .spics_io_num = 21,  // T_CS
    .queue_size = 1,
};
spi_bus_add_device(SPI2_HOST, &touch_cfg, &touch_spi);

// IRQ pin
gpio_set_direction(47, GPIO_MODE_INPUT);
gpio_set_pull_mode(47, GPIO_PULLUP_ONLY);
```

### Читання координат
```c
// Команди XPT2046:
// 0xD0 = Read X position (12-bit)
// 0x90 = Read Y position (12-bit)

uint8_t tx[3] = {0xD0, 0x00, 0x00};
uint8_t rx[3];
spi_transaction_t t = {
    .length = 24,
    .tx_buffer = tx,
    .rx_buffer = rx,
};
spi_device_transmit(touch_spi, &t);

uint16_t x = ((rx[1] << 8) | rx[2]) >> 3;  // 12-bit result
```

### Детекція дотику
```c
if(gpio_get_level(47) == 0) {  // T_IRQ = LOW = touched
    // Read coordinates
}
```

---

## 🐛 Усунення проблем

### Проблема: Чорний екран
✅ **Рішення**:
- Перевірте підключення `BL` (GPIO 14) - має бути HIGH
- Перевірте `VCC` - має бути 3.3V
- Перевірте `CS` підключення (GPIO 10)

### Проблема: Неправильні кольори
✅ **Рішення**:
```c
// Спробуйте змінити RGB порядок
.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  // або RGB
esp_lcd_panel_invert_color(panel, true);  // або false
```

### Проблема: Помилка компіляції ILI9341
✅ **Рішення**:
- ILI9341 драйвер видалено з ESP-IDF 5.5.0
- Використовуйте ST7789:
```c
#include "esp_lcd_panel_vendor.h"  // НЕ esp_lcd_panel_ili9341.h
esp_lcd_new_panel_st7789(io, &cfg, &panel);
```

### Проблема: Touch не працює (читає нулі)
⚠️ **Можливі причини**:
1. **MISO не підключено** - перевірте GPIO 13
2. **Інший touch контролер** - може бути capacitive замість resistive
3. **Окремий I2C touch** - деякі модулі використовують I2C замість SPI
4. **Несправний touch** - дисплей працює, touch - окремо

✅ **Діагностика**:
```c
// Перевірте IRQ - має змінюватись при дотику
ESP_LOGI(TAG, "T_IRQ = %d", gpio_get_level(47));  // 1→0 при дотику
```

---

## 📊 Приклад проекту

**Цей проект містить**:
- ✅ Повна ініціалізація LCD ST7789
- ✅ Малювання mountain landscape (небо, сонце, гори, дерева)
- ✅ Функції графіки: `circ()`, `tri()`, `mix()`
- ✅ Підтримка touch XPT2046 (опціонально)
- ✅ RGB565 колірна палітра
- ✅ 240x320 роздільність

### Вимкнути Touch
Якщо touch не працює, можна вимкнути:
```c
// У main.c змініть:
#define ENABLE_TOUCH 0  // 0 = тільки LCD
```

---

## 📚 Корисні посилання

- [ESP-IDF LCD Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html)
- [ST7789 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- [XPT2046 Touch Controller](http://www.xpt2046.com)
- [ESP32-S3 GPIO Matrix](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html)

---

## ✅ Перевірочний список

- [ ] VCC підключено до 3.3V (НЕ 5V!)
- [ ] GND з'єднано
- [ ] SPI піни (MOSI=11, SCK=12, CS=10)
- [ ] Контрольні піни (DC=9, RST=8, BL=14)
- [ ] Touch піни (T_CS=21, T_IRQ=47, MISO=13) - опціонально
- [ ] ESP-IDF version >= 5.0
- [ ] Використовується ST7789 драйвер (не ILI9341)
- [ ] RGB порядок встановлено правильно
- [ ] Backlight увімкнено (GPIO 14 = HIGH)

---

**Готово! 🎉 Дисплей має відображати beautiful mountain landscape!**
