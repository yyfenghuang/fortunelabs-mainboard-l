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

static const char *TAG = "TASK_SENSOR";

void task_sensor(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started.");
    sensor_reading_t reading;
    bool last_led_state = false;

    // Loop Utama Task
    while (1)
    {
        // 1. Baca data dari driver potensio dummy
        if (sensor_dummy_driver.read(&reading) == ESP_OK)
        {
            float current_volt = reading.value;
            bool current_led_state = (current_volt > VOLTAGE_THRESHOLD);

            // 2. Format & Kirim data Voltase ke Display Queue (Baris 0)
            display_msg_t msg_volt;
            msg_volt.row = 0;
            snprintf(msg_volt.text, sizeof(msg_volt.text), "VOLT: %.2f V", current_volt);
            xQueueSend(g_queue_display, &msg_volt, 0);

            // 3. Evaluasi Threshold & Kirim Status ke Actuator + Display Queue (Baris 1)
            if (current_led_state != last_led_state)
            {
                last_led_state = current_led_state;

                // Kirim perintah ke task aktuator
                xQueueSend(g_queue_actuator, &current_led_state, 0);

                // Kirim notifikasi teks ke task display
                display_msg_t msg_status;
                msg_status.row = 2; // Taruh di baris 2 agar ada jeda space di OLED
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