#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_types.h"
#include "read_sensor.h"
#include "write_display.h"
#include "write_actuator.h"

static const char *TAG = "main";

QueueHandle_t g_queue_display;
QueueHandle_t g_queue_actuator;

void app_main(void)
{
    ESP_LOGI(TAG, "Fortunelabs Mainboard PoC - booting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    g_queue_display = xQueueCreate(5, sizeof(sensor_reading_t));
    g_queue_actuator = xQueueCreate(5, sizeof(sensor_reading_t));

    if (g_queue_display == NULL || g_queue_actuator == NULL)
    {
        ESP_LOGE(TAG, "Queue creation failed");
        esp_restart();
    }
    xTaskCreate(task_read_sensor, "sensor", 2048, NULL, 3, NULL);
    xTaskCreate(task_write_display, "display", 4096, NULL, 2, NULL);
    xTaskCreate(task_write_actuator, "actuator", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "All task spawned");
}