/**
 * @file task_actuator.c
 * @brief Consumer task that drives the physical/dummy LED based on queue commands.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/app_types.h"
#include "hal/output_driver.h"
#include "drivers/output/actuator_dummy.h"
#include "esp_log.h"

static const char *TAG = "TASK_ACTUATOR";

void task_actuator(void *pvParameters)
{
    ESP_LOGI(TAG, "Actuator task started.");
    bool target_led_state = false;

    while (1)
    {
        // Block until a new command is received to change the LED state
        if (xQueueReceive(g_queue_actuator, &target_led_state, portMAX_DELAY) == pdTRUE)
        {
            // Run the actuator driver to set the LED state accordingly
            actuator_dummy_driver.set(0, target_led_state);
            ESP_LOGD(TAG, "Hardware LED synchronized to: %s", target_led_state ? "ON" : "OFF");
        }
    }
}