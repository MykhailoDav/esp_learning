#include <stdio.h>
#include <esp_log.h>
#include <ssd1306.h>
#include <i2c_scanner.h>
#include <driver/gpio.h>

static const char *TAG = "MAIN";

#define TTP223_PIN GPIO_NUM_9
#define BTN_DEB 50

static int counter = 0;

void init_ttp223(void)
{
    ESP_LOGI(TAG, "Initializing TTP223 on GPIO 9...");
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TTP223_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "GPIO 9 configured as INPUT");
    ESP_LOGI(TAG, "Connect: VCC->3.3V, GND->GND, I/O->GPIO9");
}

void my_print(const char *str) 
{
    printf("%s", str);
}

void update_display(int count)
{
    ssd1306_buffer_clear();
    ssd1306_print_str(20, 0, "ESP32-S3", false);
    ssd1306_print_str(8, 10, "Touch Counter", false);
    
    char counter_str[32];
    sprintf(counter_str, "Count: %d", count);
    
    // Center text
    uint8_t text_len = strlen(counter_str);
    uint8_t text_x = (128 - text_len * 8) / 2;
    ssd1306_print_str(text_x, 22, counter_str, false);
    
    ssd1306_display();
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== TTP223 Touch Counter ===");
    
    init_ttp223();
    
    ESP_LOGI(TAG, "Initializing SSD1306...");
    init_ssd1306();
    
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    i2c_master_bus_handle_t i2c_bus = get_i2c_bus_handle();
    if (i2c_bus != NULL) {
        ESP_LOGI(TAG, "Scanning I2C...");
        i2c_scan_devices(i2c_bus, my_print);
    }
    
    ESP_LOGI(TAG, "=== Ready! Touch the sensor ===");
    
    // Initial display
    update_display(counter);
    
    bool pState = false;
    uint32_t tmr = 0;
    
    while(1)
    {   
        bool state = gpio_get_level(TTP223_PIN); 
        
        if (pState != state) {
            if (!tmr) {
                tmr = xTaskGetTickCount() * portTICK_PERIOD_MS;  
            } else if ((xTaskGetTickCount() * portTICK_PERIOD_MS - tmr) >= BTN_DEB) { 
                pState = state; 
                
                if (state) {
                    counter++;
                    ESP_LOGI(TAG, "Touch detected! Counter: %d", counter);
                    update_display(counter);
                } else {
                    ESP_LOGI(TAG, "Released");
                }
            }
        } else {
            tmr = 0; 
        }
        
        vTaskDelay(1);
    }
}