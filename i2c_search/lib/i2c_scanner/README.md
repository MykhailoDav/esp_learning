# I2C Scanner Library

Бібліотека для сканування I2C шини та виявлення підключених пристроїв на ESP32.

## Особливості

- ✅ Сканування всіх адрес I2C (0x08 - 0x77)
- ✅ Виведення результатів у форматі таблиці
- ✅ Розпізнавання популярних I2C пристроїв
- ✅ Гнучка конфігурація GPIO та швидкості
- ✅ Підтримка декількох I2C портів
- ✅ Отримання списку знайдених адрес

## Використання

### Базовий приклад

```c
#include "i2c_scanner.h"

void app_main(void)
{
    // Використання дефолтної конфігурації
    i2c_scanner_config_t config = I2C_SCANNER_CONFIG_DEFAULT();
    
    // Ініціалізація
    esp_err_t ret = i2c_scanner_init(&config);
    if (ret != ESP_OK) {
        printf("Failed to initialize I2C scanner\n");
        return;
    }
    
    // Сканування шини
    uint8_t devices_found = i2c_scanner_scan_bus(I2C_NUM_0);
    printf("Found %d devices\n", devices_found);
}
```

### Користувацька конфігурація

```c
i2c_scanner_config_t config = {
    .i2c_port = I2C_NUM_0,
    .sda_io = GPIO_NUM_18,      // Власний GPIO для SDA
    .scl_io = GPIO_NUM_19,      // Власний GPIO для SCL
    .clk_speed = 400000,        // 400 kHz (Fast Mode)
    .enable_pullup = true,      // Внутрішні pull-up резистори
};

i2c_scanner_init(&config);
i2c_scanner_scan_bus(config.i2c_port);
```

### Перевірка конкретного пристрою

```c
uint8_t device_addr = 0x3C; // Адреса OLED дисплея

esp_err_t ret = i2c_scanner_check_device(I2C_NUM_0, device_addr);
if (ret == ESP_OK) {
    printf("Device found at 0x%02X\n", device_addr);
} else {
    printf("No device at 0x%02X\n", device_addr);
}
```

### Отримання списку пристроїв

```c
uint8_t devices[10];
uint8_t count = i2c_scanner_get_devices(I2C_NUM_0, devices, 10);

printf("Found %d devices:\n", count);
for (int i = 0; i < count; i++) {
    const char *name = i2c_scanner_get_device_name(devices[i]);
    printf("  0x%02X: %s\n", devices[i], name);
}
```

### Розпізнавання пристрою

```c
uint8_t addr = 0x68;
const char *device_name = i2c_scanner_get_device_name(addr);
printf("Device at 0x%02X is: %s\n", addr, device_name);
```

## API Функції

### `i2c_scanner_init()`
Ініціалізує I2C шину з заданою конфігурацією.

**Параметри:**
- `config` - вказівник на структуру конфігурації

**Повертає:** `ESP_OK` при успіху

### `i2c_scanner_scan_bus()`
Сканує I2C шину та виводить таблицю знайдених пристроїв.

**Параметри:**
- `i2c_port` - номер I2C порту

**Повертає:** кількість знайдених пристроїв

### `i2c_scanner_check_device()`
Перевіряє наявність пристрою за вказаною адресою.

**Параметри:**
- `i2c_port` - номер I2C порту
- `address` - 7-бітна адреса пристрою

**Повертає:** `ESP_OK` якщо пристрій знайдено

### `i2c_scanner_get_devices()`
Повертає масив адрес знайдених пристроїв.

**Параметри:**
- `i2c_port` - номер I2C порту
- `devices` - масив для зберігання адрес
- `max_devices` - максимальна кількість пристроїв

**Повертає:** кількість знайдених пристроїв

### `i2c_scanner_get_device_name()`
Повертає назву пристрою за його адресою.

**Параметри:**
- `address` - 7-бітна адреса пристрою

**Повертає:** вказівник на рядок з назвою

### `i2c_scanner_deinit()`
Деініціалізує I2C порт та звільняє ресурси.

**Параметри:**
- `i2c_port` - номер I2C порту

## Відомі пристрої

Бібліотека розпізнає наступні популярні I2C пристрої:

| Адреса | Пристрій |
|--------|----------|
| 0x1D | ADXL345 Accelerometer |
| 0x1E | HMC5883L Compass |
| 0x20-0x22 | PCF8574 I/O Expander |
| 0x23 | BH1750 Light Sensor |
| 0x24 | PN532 NFC/RFID |
| 0x27 | PCF8574 LCD Backpack |
| 0x3C, 0x3D | SSD1306 OLED Display |
| 0x40 | PCA9685 PWM Driver |
| 0x48-0x4B | ADS1115 ADC |
| 0x50-0x57 | AT24C32 EEPROM |
| 0x68 | DS3231 RTC / MPU6050 |
| 0x69 | MPU6050 / MPU9250 |
| 0x76, 0x77 | BMP280 / BME280 |

## Конфігурація за замовчуванням

```c
I2C Port:     I2C_NUM_0
SDA GPIO:     21
SCL GPIO:     22
Clock Speed:  100 kHz
Pull-up:      Enabled
```

## Приклад виводу

```
     00|01|02|03|04|05|06|07|08|09|0A|0B|0C|0D|0E|0F|
    ------------------------------------------------
 00|                        
 10| -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 20| -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 30| -- -- -- -- -- -- -- -- -- -- -- -- 3C -- -- --
 40| -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
 50| -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 60| -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
 70| -- -- -- -- -- -- -- 77 -- -- -- -- -- -- -- --

I: (12345) I2C_SCANNER: Device found at 0x3C: SSD1306 OLED Display
I: (12350) I2C_SCANNER: Device found at 0x48: ADS1115 ADC / PCF8591
I: (12355) I2C_SCANNER: Device found at 0x68: DS3231 RTC / MPU6050
I: (12360) I2C_SCANNER: Device found at 0x77: BMP280 / BME280 / BMP180
I: (12365) I2C_SCANNER: Scan complete. Found 4 device(s).
```

## Діагностика проблем

### Пристрої не знайдено
- Перевірте підключення SDA/SCL
- Переконайтеся що живлення пристроїв правильне
- Додайте зовнішні pull-up резистори (4.7kΩ)

### Помилка ініціалізації
- Перевірте що GPIO не використовуються іншими драйверами
- Переконайтеся що ESP-IDF правильно встановлено

### Неправильні адреси
- Деякі пристрої мають програмовані адреси (jumpers, switches)
- Перевірте datasheet вашого пристрою

## Ліцензія

Вільна для використання та модифікації.
