#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "app_types.h"
#include "write_display.h"
#include <string.h>

static const char *TAG = "display";

#define I2C_MASTER_SDA 18
#define I2C_MASTER_SCL 19
#define OLED_I2C_ADDRESS 0x3C

void task_write_display(void *pvParameters)
{
    // Init OLED
    SSD1306_t dev;
    i2c_master_init(&dev, I2C_MASTER_SDA, I2C_MASTER_SCL, -1);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xFF);

    ESP_LOGI(TAG, "Task started");

    sensor_reading_t reading;
    char buf[32];

    while (1)
    {
        if (xQueueReceive(g_queue_display, &reading, portMAX_DELAY) == pdTRUE)
        {
            snprintf(buf, sizeof(buf), "V: %.2fV", reading.voltage);

            ssd1306_clear_screen(&dev, false);
            ssd1306_display_text(&dev, 0, "Fortune Labs", 12, false);
            ssd1306_display_text(&dev, 2, buf, strlen(buf), false);

            ESP_LOGI(TAG, "Display: %s", buf);
        }
    }
}