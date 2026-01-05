# PN532 NFC Reader для ESP32

Проект для роботи з NFC/RFID картками та мітками за допомогою модуля PN532 та ESP32.

## 🎯 Можливості

- ✅ Читання NFC міток (ISO14443A)
- ✅ Підтримка карток Mifare Classic, Mifare Ultralight
- ✅ Виведення UID картки
- ✅ Автоматична діагностика I2C підключення
- ✅ Детальне логування

## 🔌 Підключення

### Схема

```
PN532 Module          ESP32-S3
━━━━━━━━━━━━━         ━━━━━━━━━━
VCC (3.3V)     ───►   3.3V
GND            ───►   GND
SDA            ───►   GPIO 41
SCL            ───►   GPIO 42
```

### ⚠️ ВАЖЛИВО!

1. **Режим I2C**: Переконайтеся що PN532 налаштований на I2C режим (DIP-перемикачі)
2. **Напруга**: Використовуйте тільки 3.3V! (не 5V)
3. **Pull-up резистори**: Більшість модулів мають вбудовані, але якщо ні - додайте 4.7kΩ

## 🚀 Швидкий старт

### 1. Збірка та завантаження

```bash
cd nfc
pio run -t upload
pio device monitor
```

### 2. Очікуваний вивід

При запуску ви побачите:

```
=== PN532 NFC Reader Demo ===
Using I2C: SDA=GPIO41, SCL=GPIO42

=== Scanning I2C Bus ===
     00|01|02|03|04|05|06|07|08|09|0A|0B|0C|0D|0E|0F|
    ------------------------------------------------
 00|                        
 10| -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 20| -- -- -- -- 24 -- -- -- -- -- -- -- -- -- -- --
 ...

Found 1 device(s) at addresses:
  [1] 0x24

PN532 ready! Place a card/tag on the reader...
```

### 3. Прикладіть картку

Коли прикладете NFC картку:

```
╔════════════════════════════════════════╗
║          NFC CARD DETECTED!           ║
╚════════════════════════════════════════╝
  UID: 04 A3 B2 C1 D4 5E 80
  UID Length: 7 bytes
  ATQA: 0x0044
  SAK: 0x00
  Card Type: Mifare Ultralight/NTAG
╔════════════════════════════════════════╗
```

## 📊 Підтримувані картки

| Тип картки | UID довжина | SAK | Опис |
|------------|-------------|-----|------|
| Mifare Classic 1K | 4 bytes | 0x08 | Стандартна RFID картка |
| Mifare Classic 4K | 4 bytes | 0x18 | Велика RFID картка |
| Mifare Ultralight | 7 bytes | 0x00 | Мала NFC мітка |
| NTAG213/215/216 | 7 bytes | 0x00 | NFC мітки |

## 🐛 Усунення проблем

### Помилка: "No I2C devices found"

**Рішення:**
1. Перевірте режим PN532 (має бути I2C)
2. Перевірте підключення проводів
3. Виміряйте напругу на VCC (має бути 3.3V)

### Помилка: "Failed to get firmware version"

**Рішення:**
1. Перезавантажте ESP32
2. Перевірте що адреса 0x24 видима при скануванні
3. Спробуйте зменшити швидкість I2C

### Детальна діагностика

Дивіться [TROUBLESHOOTING.md](TROUBLESHOOTING.md) для повного керівництва.

## 📁 Структура проекту

```
nfc/
├── src/
│   └── main.c                    # Основний код
├── lib/
│   └── pn532/                    # Бібліотека PN532
│       ├── pn532.h               # API
│       └── pn532.c               # Реалізація
├── platformio.ini                # Конфігурація
├── README.md                     # Цей файл
└── TROUBLESHOOTING.md            # Діагностика проблем
```

## 🔧 Налаштування GPIO

Щоб змінити GPIO піни, відредагуйте `src/main.c`:

```c
#define PN532_I2C_SDA   41  // Змініть на ваш GPIO
#define PN532_I2C_SCL   42  // Змініть на ваш GPIO
```

## 📖 API бібліотеки

### Ініціалізація

```c
pn532_t pn532;
esp_err_t ret = pn532_init(&pn532, I2C_NUM_0, SDA_GPIO, SCL_GPIO);
```

### Читання картки

```c
pn532_card_info_t card_info;
esp_err_t ret = pn532_read_passive_target(&pn532, &card_info, timeout_ms);

if (ret == ESP_OK) {
    // Картку знайдено
    printf("UID: ");
    for (int i = 0; i < card_info.uid_length; i++) {
        printf("%02X ", card_info.uid[i]);
    }
    printf("\n");
}
```

### Отримання версії firmware

```c
uint32_t version;
esp_err_t ret = pn532_get_firmware_version(&pn532, &version);
```

## 🔗 Корисні посилання

- [PN532 Datasheet](https://www.nxp.com/docs/en/user-guide/141520.pdf)
- [PN532 User Manual](https://www.nxp.com/docs/en/nxp/application-notes/AN133910.pdf)
- [Mifare Classic Datasheet](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf)

## 📝 Приклади використання

### Приклад 1: Просте читання

```c
while (1) {
    pn532_card_info_t card;
    if (pn532_read_passive_target(&pn532, &card, 1000) == ESP_OK) {
        ESP_LOGI(TAG, "Card detected! UID: %02X%02X%02X%02X",
                 card.uid[0], card.uid[1], card.uid[2], card.uid[3]);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

### Приклад 2: З контролем доступу

```c
// Дозволені UID
const uint8_t allowed_uid[] = {0x04, 0xA3, 0xB2, 0xC1};

pn532_card_info_t card;
if (pn532_read_passive_target(&pn532, &card, 1000) == ESP_OK) {
    if (memcmp(card.uid, allowed_uid, 4) == 0) {
        ESP_LOGI(TAG, "Access GRANTED!");
        // Відкрити замок, увімкнути LED, тощо
    } else {
        ESP_LOGW(TAG, "Access DENIED!");
    }
}
```

## 🎓 Навчальні ресурси

### Що таке NFC?

NFC (Near Field Communication) - технологія бездротового зв'язку малого радіусу дії (~4см).

### Як працює PN532?

1. PN532 генерує радіочастотне поле 13.56 MHz
2. Коли картка потрапляє в поле, вона живиться від цього поля
3. PN532 і картка обмінюються даними через модуляцію поля
4. PN532 передає дані в ESP32 через I2C

## 🤝 Внесок

Приймаються pull requests для:
- Підтримки інших типів карток
- Покращення документації
- Додавання прикладів
- Виправлення помилок

## 📄 Ліцензія

MIT License - вільно використовуйте та модифікуйте!

---

**Версія:** 1.0.0  
**Дата:** 31 грудня 2025  
**Автор:** ESP32 NFC Project

**Happy NFC reading! 📱✨**
