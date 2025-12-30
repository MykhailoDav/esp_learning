#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "Mountains";

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
#define SKY1 0x3A9F
#define SKY2 0x7DDF
#define SUN 0xFFE0
#define MTN1 0x528A
#define MTN2 0x3186
#define MTN3 0x2945
#define SNOW 0xFFFF
#define GRASS 0x2E05
#define TREE 0x0320
#define TRUNK 0x4A00
#define GROUND 0x2104

static uint16_t mix(uint16_t a, uint16_t b, float t) {
    uint8_t r1=(a>>11)&31, g1=(a>>5)&63, b1=a&31;
    uint8_t r2=(b>>11)&31, g2=(b>>5)&63, b2=b&31;
    return ((uint8_t)(r1+(r2-r1)*t)<<11)|((uint8_t)(g1+(g2-g1)*t)<<5)|(uint8_t)(b1+(b2-b1)*t);
}

static void circ(esp_lcd_panel_handle_t p, int cx, int cy, int r, uint16_t c) {
    for(int y=cy-r; y<=cy+r; y++)
        for(int x=cx-r; x<=cx+r; x++)
            if(y>=0&&y<H&&x>=0&&x<W&&(x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r)
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

void app_main(void) {
    ESP_LOGI(TAG,"Init");
    gpio_config_t bl={.mode=GPIO_MODE_OUTPUT,.pin_bit_mask=1ULL<<PIN_BL};
    gpio_config(&bl);
    gpio_set_level(PIN_BL,1);
    
    spi_bus_config_t bus={.sclk_io_num=PIN_SCLK,.mosi_io_num=PIN_MOSI,.miso_io_num=PIN_MISO,.quadwp_io_num=-1,.quadhd_io_num=-1,.max_transfer_sz=W*H*2};
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST,&bus,SPI_DMA_CH_AUTO));
    
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t ioc={.dc_gpio_num=PIN_DC,.cs_gpio_num=PIN_CS,.pclk_hz=LCD_CLK,.lcd_cmd_bits=8,.lcd_param_bits=8,.spi_mode=0,.trans_queue_depth=10};
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,&ioc,&io));
    
    esp_lcd_panel_handle_t p;
    esp_lcd_panel_dev_config_t pc={.reset_gpio_num=PIN_RST,.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB,.bits_per_pixel=16};
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io,&pc,&p));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(p));
    ESP_ERROR_CHECK(esp_lcd_panel_init(p));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(p,true,false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(p,true));
    
    ESP_LOGI(TAG,"Drawing...");
    uint16_t *b=heap_caps_malloc(W*2,MALLOC_CAP_DMA);
    
    // Sky
    for(int y=0;y<180;y++){
        uint16_t c=mix(SKY1,SKY2,(float)y/180);
        for(int x=0;x<W;x++)b[x]=c;
        esp_lcd_panel_draw_bitmap(p,0,y,W,y+1,b);
    }
    
    circ(p,60,50,25,SUN);
    tri(p,0,180,80,120,160,180,MTN1);
    tri(p,100,180,180,130,240,180,MTN1);
    tri(p,-20,200,60,140,140,200,MTN2);
    tri(p,90,200,170,150,250,200,MTN2);
    tri(p,20,220,120,160,220,220,MTN3);
    tri(p,95,180,120,160,145,180,SNOW);
    
    // Grass
    for(int y=220;y<290;y++){
        for(int x=0;x<W;x++)b[x]=GRASS;
        esp_lcd_panel_draw_bitmap(p,0,y,W,y+1,b);
    }
    
    // Trees
    tri(p,30,290,40,260,50,290,TREE);
    tri(p,32,280,40,255,48,280,TREE);
    uint16_t trunk=TRUNK;
    for(int y=285;y<295;y++)for(int x=38;x<42;x++)esp_lcd_panel_draw_bitmap(p,x,y,x+1,y+1,&trunk);
    tri(p,190,290,200,265,210,290,TREE);
    tri(p,192,280,200,260,208,280,TREE);
    for(int y=285;y<295;y++)for(int x=198;x<202;x++)esp_lcd_panel_draw_bitmap(p,x,y,x+1,y+1,&trunk);
    
    // Ground
    for(int y=290;y<H;y++){
        for(int x=0;x<W;x++)b[x]=GROUND;
        esp_lcd_panel_draw_bitmap(p,0,y,W,y+1,b);
    }
    
    free(b);
    ESP_LOGI(TAG,"Done!");
    
    while(1)vTaskDelay(pdMS_TO_TICKS(1000));
}
