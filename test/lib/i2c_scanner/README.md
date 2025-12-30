# I2C Scanner Library for ESP32

Бібліотека для сканування I2C шини та пошуку підключених пристроїв.

## Функції

### `void i2c_scan_devices(i2c_master_bus_handle_t i2c_bus, void (*lprint)(const char *))`
Сканує I2C шину та виводить таблицю знайдених пристроїв через callback функцію.

**Параметри:**
- `i2c_bus` - хендл I2C master bus
- `lprint` - callback функція для виводу (наприклад, printf)

**Приклад:**
```c
#include "i2c_scanner.h"

void my_print(const char *str) {
    printf("%s", str);
}

i2c_scan_devices(i2c_master_bus, my_print);
```

### `uint8_t i2c_scan_get_addresses(i2c_master_bus_handle_t i2c_bus, uint8_t *addresses, uint8_t max_devices)`
Сканує I2C шину та повертає масив знайдених адрес.

**Параметри:**
- `i2c_bus` - хендл I2C master bus
- `addresses` - масив для збереження знайдених адрес (мінімум 128 байт)
- `max_devices` - максимальна кількість пристроїв для пошуку

**Повертає:**
- Кількість знайдених пристроїв

**Приклад:**
```c
#include "i2c_scanner.h"

uint8_t addresses[10];
uint8_t count = i2c_scan_get_addresses(i2c_master_bus, addresses, 10);

for(int i = 0; i < count; i++) {
    printf("Device %d: 0x%02X\n", i, addresses[i]);
}
```

## Використання

1. Додайте бібліотеку до свого проекту
2. Включіть хедер файл: `#include "i2c_scanner.h"`
3. Ініціалізуйте I2C bus
4. Викличте функцію сканування

## Примітки

- Бібліотека використовує ESP-IDF I2C master driver
- Таймаут для кожного пристрою: 50 мс
- Між пробами є затримка 2 мс для стабільності
- Сканування відбувається для адрес від 0x03 до 0x77
