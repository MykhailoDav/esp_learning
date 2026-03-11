# LAB 2.2: I2C Protocol on ESP32

## Task Adaptation
- **Original:** 2x BMP280
- **Using:** 1x BMP280 + 1x HTU21 (SHT21) + 1x SSD1306 OLED (128×32)
- **Reason:** Demonstrates I2C bus with multiple devices and live data display on screen

---

## 📋 Required Components

| Component | Qty | Note |
|-----------|-----|------|
| ESP32 NodeMCU | 1 | GPIO41 (SDA), GPIO42 (SCL) |
| BMP280 | 1 | I2C address: 0x76 |
| HTU21 (SHT21) | 1 | I2C address: 0x40 (fixed) |
| SSD1306 OLED 128×32 | 1 | I2C address: 0x3C |
| Pull-up resistors | 2 | 4.7kΩ (optional — often built into modules) |
| Jumper wires | - | For connections |

---

## 🔌 Wiring Diagram

```
       ESP32 NodeMCU
    ┌─────────────────┐
    │                 │
    │ GPIO41 (SDA)────┼──────┬──────────────┬──────────────┐
    │                 │      │              │              │
    │ GPIO42 (SCL)────┼──────┼──────────────┼──────────────┤
    │                 │      │              │              │
    │ GND─────────────┼──────┼──┐        ┌──┼──┐        ┌──┤
    │                 │      │  │        │  │  │        │  │
    │ 3.3V────────────┼──────┼──┴────────┴──┼──┴────────┴──┤
    │                 │      │              │              │
    └─────────────────┘      │              │              │
                             │              │              │
                    ┌────────┴──┐   ┌───────┴───┐   ┌──────┴──┐
                    │  BMP280   │   │   HTU21   │   │ SSD1306 │
                    │  VCC→3.3V │   │  VCC→3.3V │   │ VCC→3.3V│
                    │  GND→GND  │   │  GND→GND  │   │ GND→GND │
                    │  SCL→42   │   │  SCL→42   │   │ SCL→42  │
                    │  SDA→41   │   │  SDA→41   │   │ SDA→41  │
                    └───────────┘   └───────────┘   └─────────┘
```

### Pin Connections

#### BMP280
| BMP280 Pin | → | ESP32 GPIO |
|------------|---|------------|
| VCC | → | 3.3V |
| GND | → | GND |
| SCL | → | GPIO42 |
| SDA | → | GPIO41 |

#### HTU21 (SHT21)
| HTU21 Pin | → | ESP32 GPIO |
|-----------|---|------------|
| VCC | → | 3.3V |
| GND | → | GND |
| SCL | → | GPIO42 |
| SDA | → | GPIO41 |

#### SSD1306 OLED
| SSD1306 Pin | → | ESP32 GPIO |
|-------------|---|------------|
| VCC | → | 3.3V |
| GND | → | GND |
| SCL | → | GPIO42 |
| SDA | → | GPIO41 |

---

## 📍 I2C Addresses

| Device | Address | Note |
|--------|---------|------|
| BMP280 | 0x76 | Default; 0x77 if SDO is tied to VCC |
| HTU21 | 0x40 | Fixed address |
| SSD1306 | 0x3C | Most common; 0x3D if SA0=HIGH |

---

## 🚀 Quick Start

### 1. Connect components as shown above

### 2. Upload firmware to ESP32
```bash
cd /Users/mykhailodavydenko/Documents/UNI/Embeded/esp_learning/lab_2/lab_2.2_i2c
pio run -t upload -e 4d_systems_esp32s3_gen4_r8n16
```

### 3. Open serial monitor
```bash
pio device monitor -b 115200
```

### 4. Check output
- `scanI2C()` will find all devices on the bus
- `readBMP280()` outputs temperature and pressure
- `readHTU21()` outputs temperature and humidity
- Temperature comparison between both sensors

---

## 📊 Expected Output

```
===== I2C Bus Scan =====
  0x3C  ← SSD1306
  0x40  ← HTU21
  0x76  ← BMP280
Found: 3

===== Init =====
BMP280 chip ID: 0x58
✓ BMP280 OK
✓ HTU21 OK
✓ SSD1306 OK

========== t=5s ==========
[BMP280]  Temp: 23.45 C  Pressure: 1013.25 hPa
[HTU21]   Temp: 23.12 C  Humidity: 45.67 %
[COMPARE] dT = 0.33 C
```

---

## 🔧 Troubleshooting

### BMP280 not found:
1. Try address 0x77 (change `#define BMP280_ADDRESS 0x77` in code)
2. Check SCL/SDA wires
3. Run `scanI2C()` to diagnose the bus

### HTU21 not found:
1. HTU21 has a fixed address of 0x40 — no alternatives
2. Check power and I2C wires
3. Allow ~100 ms startup time after power-on

---

## 📈 Comparison with Reference Values

### Atmospheric Pressure
- **Sea-level standard:** 1013.25 hPa
- **Your BMP280 reading:** Varies with altitude and weather
- **Verify:** Compare with weather.com for your location

### Temperature
- **Difference between BMP280 and HTU21:** Typically 0.3–1.0 °C (normal)
- **BMP280 may read slightly higher** due to self-heating
- **Verify:** Compare with a reference thermometer

### Humidity
- **HTU21 humidity:** Depends on environment
- **Typical indoor range:** 30–60 %

---

## 💡 Lab Report Checklist

1. **Wiring diagram** (copy the diagram above or draw your own)
2. **I2C address table** for all connected devices
3. **Measured values:**
   - Temperature (BMP280, HTU21)
   - Pressure (BMP280)
   - Humidity (HTU21)
4. **Comparison with reference values** from external sources
5. **Analysis of sensor discrepancies**
6. **Conclusion:** How I2C allows reading from multiple devices on a single bus

---

## 🎯 Optional Extensions

1. **Read sensor address programmatically** from EEPROM
2. **Apply BMP280 compensation** for calibration offset
3. **Calculate altitude** from pressure reading
4. **Log data to flash** for later analysis
5. **Plot temperature/pressure over time**

---

## 📚 References

- [Adafruit BMP280 Library](https://github.com/adafruit/Adafruit_BMP280_Library)
- [Adafruit HTU21DF Library](https://github.com/adafruit/Adafruit_HTU21DF_Library)
- [I2C on ESP32 — Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)

---

**Date:** 2026-03-07  
**Lab:** I2C Protocol Investigation  
**Note:** Adapted for available hardware components
