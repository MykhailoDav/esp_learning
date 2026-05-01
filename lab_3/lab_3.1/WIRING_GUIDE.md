# Lab 3.1 — ESP32 Interrupts

## Overview

This project demonstrates **hardware interrupts** on ESP32-S3. Two buttons trigger ISR (Interrupt Service Routine) handlers that set flags, which are then processed in `loop()`.

### What happens

| Button | Action |
|--------|--------|
| **Button 1** | Reads photoresistor value via ADC, prints light level, blinks LED1 3 times |
| **Button 2** | Increments press counter, toggles LED2 on/off, cycles LED3 brightness (PWM: 0→20→40→60→80→100→0%) |

## Wiring Diagram

```
ESP32-S3 Pin    Component              Connection
──────────────────────────────────────────────────────
GPIO4           Button 1               Other leg → GND
GPIO5           Button 2               Other leg → GND
GPIO6           LED1 (blink)           → 220Ω → LED anode (+) → GND
GPIO7           LED2 (toggle)          → 220Ω → LED anode (+) → GND
GPIO15          LED3 (PWM brightness)  → 220Ω → LED anode (+) → GND
GPIO3           Photoresistor          See voltage divider below
3.3V            Photoresistor power
GND             Common ground
```

### Photoresistor — Voltage Divider

```
3.3V ─── [Photoresistor] ───┬─── GPIO3
                             │
                          [10kΩ]
                             │
                            GND
```

### Buttons — Internal Pull-Up

No external resistor needed — `INPUT_PULLUP` is used.

```
GPIOx ──── [Button] ──── GND
```

When not pressed: pin reads HIGH. When pressed: pin reads LOW → triggers FALLING interrupt.

### LEDs

```
GPIOx ──── [220Ω] ──── LED(+) ──── LED(−) ──── GND
```

The longer leg of the LED is the anode (+), shorter is cathode (−).

## Key Concepts

- **Interrupt**: CPU immediately stops `loop()`, runs the ISR function, then returns
- **IRAM_ATTR**: Stores ISR in RAM for fast execution (required on ESP32)
- **volatile**: Tells compiler the variable can change at any time (from ISR)
- **Debounce**: Ignores repeated triggers within 200ms to filter mechanical button bounce
- **FALLING**: Interrupt fires on HIGH→LOW transition (button press with pull-up)
- **PWM (ledcWrite)**: Controls LED brightness by rapidly switching on/off at varying duty cycles
