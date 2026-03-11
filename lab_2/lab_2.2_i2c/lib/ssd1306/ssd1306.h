#pragma once

#include <Arduino.h>
#include <Wire.h>

/* ---- SSD1306 command constants (from Bosch/Solomon SSD1306 datasheet) ---- */
#define SSD1306_CMD         0x00  // control byte: command follows
#define SSD1306_DATA        0x40  // control byte: data follows

#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_CONTRAST            0x81
#define SSD1306_SEGREMAP            0xA0  // | 0x01 to flip H
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_COMSCANDEC          0xC8  // flip V
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_CHARGEPUMP          0x8D

/**
 * @brief Minimal Arduino Wire-based SSD1306 driver.
 *
 * Supports 128×32 and 128×64 displays.
 * Maintains a RAM framebuffer; call display() to push it to the OLED.
 * Uses the same 8×8 font as the test/lib/ssd1306 project.
 *
 * Usage example (128×32 at 0x3C on Wire with SDA=41, SCL=42):
 *   Wire.begin(41, 42);
 *   SSD1306 oled;
 *   oled.begin();
 *   oled.clear();
 *   oled.print(0, 0, "Hello!");
 *   oled.display();
 */
class SSD1306 {
public:
    /**
     * @param addr   I2C address (0x3C or 0x3D)
     * @param width  Display width in pixels  (default 128)
     * @param height Display height in pixels (default 32)
     */
    SSD1306(uint8_t addr = 0x3C, uint8_t width = 128, uint8_t height = 32);
    ~SSD1306();

    /**
     * @brief Initialize the display on the given Wire bus.
     * @return true on success.
     */
    bool begin(TwoWire &wire = Wire);

    /** @brief Fill framebuffer with zeros (all pixels off). */
    void clear();

    /**
     * @brief Push the entire framebuffer to the display RAM.
     * Sends data in 16-byte chunks to stay within I2C buffer limits.
     */
    void display();

    /**
     * @brief Render ASCII text using 8×8 font into the framebuffer.
     * @param x     Pixel column (0 … width-1)
     * @param y     Pixel row    (0 … height-1), snapped to 8-pixel page boundary
     * @param text  Null-terminated ASCII string
     */
    void print(uint8_t x, uint8_t y, const char *text);

    /**
     * @brief Render a formatted string (printf-style) into the framebuffer.
     * @param x     Pixel column
     * @param y     Pixel row
     * @param fmt   printf format string
     */
    void printf(uint8_t x, uint8_t y, const char *fmt, ...);

private:
    uint8_t   _addr;
    uint8_t   _width;
    uint8_t   _height;
    uint8_t   _pages;
    uint8_t  *_buf;       // framebuffer [_pages][_width]
    TwoWire  *_wire;

    void cmd(uint8_t c);
    void cmd2(uint8_t c, uint8_t arg);
    void cmd3(uint8_t c, uint8_t a1, uint8_t a2);
};
