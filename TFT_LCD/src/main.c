#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "NY2026";

#define LCD_HOST SPI2_HOST
#define LCD_CLK 40000000
#define PIN_MOSI 11
#define PIN_MISO 13
#define PIN_SCLK 12
#define PIN_CS 10
#define PIN_DC 9
#define PIN_RST 8
#define PIN_BL 14
#define PIN_TCH_CS 21
#define PIN_TCH_IRQ 47
#define W 240
#define H 320

// Colors RGB565
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define ORANGE 0xFD20
#define PINK 0xF81F
#define PURPLE 0x780F
#define DARK_BLUE 0x0010
#define DARK_GREEN 0x0300
#define BROWN 0x4A00
#define GOLD 0xFEA0

// Snowflake structure
typedef struct {
    float x, y, speed;
    uint8_t size;
} Snowflake;

#define MAX_SNOW 50
static Snowflake snow[MAX_SNOW];

static void circ(esp_lcd_panel_handle_t p, int cx, int cy, int r, uint16_t c) {
    for(int y=cy-r; y<=cy+r; y++)
        for(int x=cx-r; x<=cx+r; x++)
            if(y>=0&&y<H&&x>=0&&x<W&&(x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r)
                esp_lcd_panel_draw_bitmap(p,x,y,x+1,y+1,&c);
}

static void rect(esp_lcd_panel_handle_t p, int x1, int y1, int x2, int y2, uint16_t c) {
    for(int y=y1; y<=y2; y++)
        for(int x=x1; x<=x2; x++)
            if(x>=0&&x<W&&y>=0&&y<H)
                esp_lcd_panel_draw_bitmap(p,x,y,x+1,y+1,&c);
}

static void tri(esp_lcd_panel_handle_t p, int x1, int y1, int x2, int y2, int x3, int y3, uint16_t c) {
    if(y1>y2){int t=y1;y1=y2;y2=t;t=x1;x1=x2;x2=t;}
    if(y1>y3){int t=y1;y1=y3;y3=t;t=x1;x1=x3;x3=t;}
    if(y2>y3){int t=y2;y2=y3;y3=t;t=x2;x2=x3;x3=t;}
    for(int y=y1;y<=y3;y++){
        if(y<0||y>=H)continue;
        float xa=(y3!=y1)?x1+(float)(y-y1)*(x3-x1)/(y3-y1):x1;
        float xb=(y<y2)?((y2!=y1)?x1+(float)(y-y1)*(x2-x1)/(y2-y1):x1):((y3!=y2)?x2+(float)(y-y2)*(x3-x2)/(y3-y2):x2);
        int xs=(xa<xb)?xa:xb, xe=(xa>xb)?xa:xb;
        for(int x=xs;x<=xe;x++)if(x>=0&&x<W)esp_lcd_panel_draw_bitmap(p,x,y,x+1,y+1,&c);
    }
}

// Initialize snowflakes
static void init_snow(void) {
    for(int i=0; i<MAX_SNOW; i++) {
        snow[i].x = esp_random() % W;
        snow[i].y = esp_random() % H;
        snow[i].speed = 0.5 + (esp_random() % 20) / 10.0;
        snow[i].size = 1 + (esp_random() % 3);
    }
}

// Update and draw snow
static void update_snow(esp_lcd_panel_handle_t p) {
    for(int i=0; i<MAX_SNOW; i++) {
        snow[i].y += snow[i].speed;
        if(snow[i].y > H) {
            snow[i].y = 0;
            snow[i].x = esp_random() % W;
        }
        circ(p, snow[i].x, snow[i].y, snow[i].size, WHITE);
    }
}

// Draw Christmas tree
static void draw_tree(esp_lcd_panel_handle_t p, int cx, int by, bool lights_on) {
    // Star
    circ(p, cx, by-90, 5, GOLD);
    
    // Tree layers (use bright GREEN for visibility)
    tri(p, cx-30, by-70, cx, by-90, cx+30, by-70, GREEN);
    tri(p, cx-35, by-50, cx, by-75, cx+35, by-50, GREEN);
    tri(p, cx-40, by-30, cx, by-60, cx+40, by-30, GREEN);
    tri(p, cx-45, by-10, cx, by-45, cx+45, by-10, GREEN);
    
    // Trunk
    rect(p, cx-8, by-10, cx+8, by, BROWN);
    
    // Lights (if on)
    if(lights_on) {
        uint16_t colors[] = {RED, YELLOW, BLUE, MAGENTA, CYAN};
        circ(p, cx-20, by-65, 3, colors[0]);
        circ(p, cx+15, by-68, 3, colors[1]);
        circ(p, cx-25, by-45, 3, colors[2]);
        circ(p, cx+20, by-50, 3, colors[3]);
        circ(p, cx-30, by-25, 3, colors[4]);
        circ(p, cx+25, by-30, 3, colors[0]);
        circ(p, cx-35, by-12, 3, colors[1]);
        circ(p, cx+30, by-15, 3, colors[2]);
    }
}

// Glyph indices order reference:
// 0:А 1:В 2:Г 3:І 4:Е 5:Ї 6:Є 7:К 8:Л 9:М 10:Н 11:О 12:Р 13:С 14:Т 15:У 16:Х 17:Ю 18:Я 19:Б 20:space 21:Ь 22:И 23:П 24:Д 25:Й 26:З 27:Ч 28:comma 29:apostrophe 30:!
static void draw_glyph(esp_lcd_panel_handle_t p, int x, int y, uint8_t glyph, uint16_t color) {
    static const uint8_t font[][5] = {
        {0x7E,0x11,0x11,0x11,0x7E}, // А
        {0x7F,0x49,0x49,0x49,0x36}, // В
        {0x7F,0x01,0x01,0x01,0x01}, // Г
        {0x7F,0x10,0x10,0x10,0x7F}, // І
        {0x7F,0x49,0x49,0x49,0x41}, // Е
        {0x00,0x65,0x7F,0x65,0x00}, // Ї
        {0x7F,0x49,0x7F,0x49,0x7F}, // Є (approx)
        {0x7F,0x08,0x14,0x22,0x41}, // К
        {0x7F,0x40,0x40,0x40,0x40}, // Л
        {0x7F,0x02,0x0C,0x02,0x7F}, // М
        {0x7F,0x04,0x08,0x10,0x7F}, // Н
        {0x3E,0x41,0x41,0x41,0x3E}, // О
        {0x7F,0x09,0x09,0x09,0x06}, // Р
        {0x46,0x49,0x49,0x49,0x31}, // С
        {0x01,0x01,0x7F,0x01,0x01}, // Т
        {0x3F,0x40,0x40,0x40,0x3F}, // У
        {0x63,0x14,0x08,0x14,0x63}, // Х
        {0x7F,0x40,0x7C,0x40,0x7F}, // Ю
        {0x7C,0x0A,0x09,0x0A,0x7C}, // Я
        {0x7F,0x48,0x48,0x48,0x30}, // Б
        {0x00,0x00,0x00,0x00,0x00}, // space
        {0x7F,0x49,0x49,0x49,0x36}, // Ь
        {0x41,0x22,0x7F,0x22,0x41}, // И
        {0x41,0x41,0x7F,0x41,0x41}, // П
        {0x7F,0x10,0x08,0x04,0x7F}, // Д
        {0x63,0x55,0x49,0x41,0x41}, // Й
        {0x61,0x51,0x49,0x45,0x43}, // З
        {0x46,0x49,0x49,0x29,0x1E}, // Ч
        {0x00,0x00,0x06,0x06,0x00}, // , (comma)
        {0x00,0x00,0x08,0x00,0x00}, // ' (apostrophe)
        {0x00,0x00,0x5F,0x00,0x00}, // !
    };
    if(glyph >= sizeof(font)/sizeof(font[0])) return;
    for(int i=0; i<5; i++) {
        for(int j=0; j<8; j++) {
            if(font[glyph][i] & (1<<j)) {
                esp_lcd_panel_draw_bitmap(p, x+i, y+j, x+i+1, y+j+1, &color);
            }
        }
    }
}

static void draw_glyphs(esp_lcd_panel_handle_t p, int x, int y, const uint8_t *glyphs, int len, uint16_t color) {
    int cx = x;
    for(int i=0;i<len;i++) {
        draw_glyph(p, cx, y, glyphs[i], color);
        cx += 6;
    }
}


void app_main(void) {
    ESP_LOGI(TAG,"Happy New Year 2026!");
    
    // Initialize GPIO for backlight
    gpio_config_t bl={.mode=GPIO_MODE_OUTPUT,.pin_bit_mask=1ULL<<PIN_BL};
    gpio_config(&bl);
    gpio_set_level(PIN_BL,1);
    
    // Initialize SPI bus
    spi_bus_config_t bus={.sclk_io_num=PIN_SCLK,.mosi_io_num=PIN_MOSI,.miso_io_num=PIN_MISO,.quadwp_io_num=-1,.quadhd_io_num=-1,.max_transfer_sz=W*H*2};
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST,&bus,SPI_DMA_CH_AUTO));
    
    // Initialize LCD panel IO
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t ioc={.dc_gpio_num=PIN_DC,.cs_gpio_num=PIN_CS,.pclk_hz=LCD_CLK,.lcd_cmd_bits=8,.lcd_param_bits=8,.spi_mode=0,.trans_queue_depth=10};
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,&ioc,&io));
    
    // Initialize LCD panel
    esp_lcd_panel_handle_t p;
    // Many ST7789 panels use BGR order; switch to BGR so colors display correctly
    esp_lcd_panel_dev_config_t pc={.reset_gpio_num=PIN_RST,.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_BGR,.bits_per_pixel=16};
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io,&pc,&p));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(p));
    ESP_ERROR_CHECK(esp_lcd_panel_init(p));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(p,true,false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(p,true));
    
    // Initialize snow
    init_snow();
    
    ESP_LOGI(TAG,"Starting animation...");
    
    uint16_t *buffer = heap_caps_malloc(W*2, MALLOC_CAP_DMA);
    
    // Clear screen with dark blue night sky
    for(int y=0; y<H; y++) {
        for(int x=0; x<W; x++) buffer[x] = DARK_BLUE;
        esp_lcd_panel_draw_bitmap(p, 0, y, W, y+1, buffer);
    }

    // Draw snow on ground
    for(int y=H-30; y<H; y++) {
        for(int x=0; x<W; x++) buffer[x] = WHITE;
        esp_lcd_panel_draw_bitmap(p, 0, y, W, y+1, buffer);
    }

    // Draw Christmas tree (static) with lights ON
    draw_tree(p, W/2, H-30, true);

 

    // Keep idle; snowflakes optional static overlay (no animation)
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // not reached
    // free(buffer);
}
