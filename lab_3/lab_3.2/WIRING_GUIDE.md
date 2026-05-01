# Lab 3.2 — ESP32 Hardware Timers

## Overview

Two hardware timers periodically read a photoresistor via ADC at different intervals:

| Timer | Interval | Purpose |
|-------|----------|---------|
| Timer 1 | 1 second | Frequent light level readings |
| Timer 2 | 3 seconds | Less frequent readings for comparison |

## Wiring

```
ESP32-S3 Pin    Component              Connection
──────────────────────────────────────────────────────
GPIO3           Photoresistor          Voltage divider (see below)
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

> The original task specified 100Ω, but with such a low value the ADC readings were stuck near max (~4095). Using 10kΩ (same as Lab 3.1) gives a usable range across different light levels.

## Key Concepts

- **Hardware Timer**: ESP32 has 4 hardware timers (0–3). Each runs independently from CPU, counting clock ticks
- **Prescaler = 80**: ESP32 base clock is 80 MHz. Dividing by 80 gives 1 MHz → 1 tick = 1 µs
- **timerBegin(id, prescaler, countUp)**: Initializes timer with given prescaler
- **timerAlarmWrite(timer, µs, autoReload)**: Sets the alarm interval; `autoReload=true` means it repeats
- **timerAttachInterrupt**: Connects a timer to an ISR function
- **ISR flag pattern**: ISR only sets a `volatile bool` flag; actual ADC reading happens in `loop()` (ISRs must be fast)
