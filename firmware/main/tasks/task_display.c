/**
 * @file task_display.c
 * @brief Consumer task that listens for text updates and renders them to the SSD1306.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/app_types.h"
#include "hal/display_driver.h"
#include "drivers/ic/ssd1306.h"
#include "esp_log.h"

static const char *TAG = "TASK_DISPLAY";

void task_display(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task started.");
    display_msg_t incoming_msg;

    while (1)
    {
        // Memblokir diri sampai ada teks baru yang siap dicetak ke layar
        if (xQueueReceive(g_queue_display, &incoming_msg, portMAX_DELAY) == pdTRUE)
        {
            // Tembak string langsung ke row target pada OLED fisik
            ssd1306_driver.show_text(incoming_msg.row, incoming_msg.text);
        }
    }
}