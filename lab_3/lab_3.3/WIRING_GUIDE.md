# Lab 3.3 — Summary: Timer + Button Interrupt + BMP280

## Overview

| Feature | Description |
|---------|------------|
| **Hardware Timer** | Every 5 s reads BMP280 (temperature + pressure) via I2C, blinks LED1 |
| **Button Interrupt** | Measures hold duration; after release, LED2 lights for the same time |

## Wiring

```
ESP32-S3 Pin    Component              Connection
──────────────────────────────────────────────────────
GPIO41 (SDA)    BMP280 SDA             I2C data
GPIO42 (SCL)    BMP280 SCL             I2C clock
3.3V            BMP280 VCC             Power
GND             BMP280 GND + SDO       Ground (SDO→GND = addr 0x76)

GPIO4           Button                 Other leg → GND
GPIO6           LED1 (timer blink)     → 220Ω → LED → GND
GPIO7           LED2 (button duration) → 220Ω → LED → GND
GND             Common ground
```

### BMP280

```
ESP32-S3         BMP280
────────         ──────
GPIO41  ──────── SDA
GPIO42  ──────── SCL
3.3V    ──────── VCC
GND     ──────── GND
GND     ──────── SDO   (address = 0x76)
```

### Button (Internal Pull-Up)

```
GPIO4 ──── [Button] ──── GND
```

No external resistor needed — `INPUT_PULLUP` is used.  
Interrupt mode: `CHANGE` (fires on both press and release to measure duration).

### LEDs

```
GPIOx ──── [220Ω] ──── LED(+) ──── LED(−) ──── GND
```

## Key Concepts

- **Hardware Timer**: prescaler 80 → 1 tick = 1 µs, alarm at 5 000 000 µs = 5 s
- **ISR flag pattern**: ISR sets a `volatile bool` flag; heavy work (I2C read, Serial print) runs in `loop()`
- **CHANGE interrupt**: fires on both FALLING (press) and RISING (release), allowing duration measurement via `millis()` difference
- **BMP280 via I2C**: `Wire.begin(SDA, SCL)` → `bmp.begin(Wire)` → `readTemperature()` / `readPressure()`
