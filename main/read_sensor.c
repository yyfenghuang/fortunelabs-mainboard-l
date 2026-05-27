#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "app_types.h"
#include "read_sensor.h"

static const char *TAG = "sensor";

#define SENSOR_SAMPLE_MS 500

void task_read_sensor(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started — dummy mode");

    sensor_reading_t reading;
    TickType_t last_wake = xTaskGetTickCount();
    float dummy_voltage = 0.0f;

    while (1)
    {
        // Simulasi tegangan naik dari 0 ke 3.3V lalu reset
        dummy_voltage += 0.1f;
        if (dummy_voltage > 3.3f)
            dummy_voltage = 0.0f;

        reading.voltage = dummy_voltage;
        reading.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        ESP_LOGI(TAG, "voltage=%.2fV", reading.voltage);

        xQueueSend(g_queue_display, &reading, 0);
        xQueueSend(g_queue_actuator, &reading, 0);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_SAMPLE_MS));
    }
}