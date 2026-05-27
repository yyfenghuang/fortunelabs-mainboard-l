#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "app_types.h"
#include "write_actuator.h"

static const char *TAG = "actuator";

#define LED_GPIO_PIN GPIO_NUM_2
#define THRESHOLD_VOLT 2.5f

void task_write_actuator(void *pvParameters)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    ESP_LOGI(TAG, "Actuator task started - Threshoold: %.1fV", THRESHOLD_VOLT);

    sensor_reading_t reading;

    while (1)
    {
        if (xQueueReceive(g_queue_actuator, &reading, portMAX_DELAY) == pdTRUE)
        {
            if (reading.voltage > THRESHOLD_VOLT)
            {
                gpio_set_level(LED_GPIO_PIN, 1);
                ESP_LOGI(TAG, "LED ON - %.2fV", reading.voltage);
            }
            else
            {
                gpio_set_level(LED_GPIO_PIN, 0);
                ESP_LOGI(TAG, "LED OFF - %.2fV", reading.voltage);
            }
        }
    }
}