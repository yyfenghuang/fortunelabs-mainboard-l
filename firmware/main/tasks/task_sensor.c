/**
 * @file task_sensor.c
 * @brief Periodically samples the dummy potentiometer and dispatches state updates.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/app_types.h"
#include "hal/sensor_driver.h"
#include "drivers/sensor/sensor_dummy.h"
#include "esp_log.h"
#include <stdio.h>

#define SENSOR_PERIOD_MS 200
#define VOLTAGE_THRESHOLD 2.5f

static const char *TAG = "task_sensor";
static float s_latest_hardware_voltage = 0.0f;

float task_sensor_get_latest_voltage(void)
{
    return s_latest_hardware_voltage;
}

void task_sensor(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started.");
    sensor_reading_t reading;
    bool last_led_state = false;

    // Main loop: Read sensor, send updates to display and actuator tasks
    while (1)
    {
        // 1. Read data from the dummy potentiometer driver
        if (sensor_dummy_driver.read(&reading) == ESP_OK)
        {
            float current_volt = reading.value;
            s_latest_hardware_voltage = current_volt;
            bool current_led_state = (current_volt > VOLTAGE_THRESHOLD);

            // 2. Send voltage reading to display task via queue (row 0)
            display_msg_t msg_volt;
            msg_volt.row = 0;
            snprintf(msg_volt.text, sizeof(msg_volt.text), "VOLT: %.2f V", current_volt);
            xQueueSend(g_queue_display, &msg_volt, 0);

            // 3. Evaluate threshold & send status to actuator + display queue (row 1)
            if (current_led_state != last_led_state)
            {
                last_led_state = current_led_state;

                // Send command to actuator task
                xQueueSend(g_queue_actuator, &current_led_state, 0);

                // Send text notification to display task
                display_msg_t msg_status;
                msg_status.row = 2; // Place on row 2 to create a space on the OLED
                if (current_led_state)
                {
                    snprintf(msg_status.text, sizeof(msg_status.text), "OVER THRESHOLD!");
                }
                else
                {
                    snprintf(msg_status.text, sizeof(msg_status.text), "SYSTEM OK");
                }
                xQueueSend(g_queue_display, &msg_status, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}