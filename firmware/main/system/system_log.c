#include "system/system_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char *TAG = "system_log";

// One queued log line
typedef struct
{
    char buf[SYS_LOG_LINE_MAX];
    size_t len;
} log_line_t;

static QueueHandle_t s_queue = NULL;
static vprintf_like_t s_orig_vprintf = NULL;   // default UART driver
static volatile sys_log_sink_fn s_sink = NULL; // remote sink
static volatile bool s_forwarding = false;     // recursion guard
static bool s_initialized = false;

//* Internal
// esp_log hook, always write to serial, then queue for the sink
static int log_vprintf(const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    // 1. Serial output via original writer
    int n = s_orig_vprintf ? s_orig_vprintf(fmt, args) : vprintf(fmt, args);
    // 2. Forward copy to sink queue
    if (s_sink != NULL && s_queue != NULL && !s_forwarding)
    {
        log_line_t line;
        int written = vsnprintf(line.buf, sizeof(line.buf), fmt, args_copy);
        if (written > 0)
        {
            line.len = (written < (int)sizeof(line.buf))
                           ? (size_t)written
                           : sizeof(line.buf) - 1;
            // Non blockingL drop on full so logging never stalls a task
            (void)xQueueSend(s_queue, &line, 0);
        }
    }
    va_end(args_copy);
    return n;
}

// Drain the queue and deliver lines to registered sink
static void task_log_forwarder(void *pvParameters)
{
    (void)pvParameters;
    log_line_t line;

    while (1)
    {
        if (xQueueReceive(s_queue, &line, portMAX_DELAY) == pdTRUE)
        {
            sys_log_sink_fn sink = s_sink;
            if (sink != NULL)
            {
                s_forwarding = true;
                sink(line.buf, line.len);
                s_forwarding = false;
            }
        }
    }
}

//* Logging Lifecycle
esp_err_t sys_log_init(esp_log_level_t default_level)
{
    esp_log_level_set("*", default_level);

    s_queue = xQueueCreate(SYS_LOG_QUEUE_DEPTH, sizeof(log_line_t));
    if (s_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create log queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(task_log_forwarder, "log_fwd", 4096, NULL, 1, NULL);
    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create forwarder task");
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    // Install the hook last; save default writer to chain serial output
    s_orig_vprintf = esp_log_set_vprintf(log_vprintf);
    s_initialized = true;

    ESP_LOGI(TAG, "Logging ready (level=%d, sink idle)", default_level);
    return ESP_OK;
}

//* Level Control
void sys_log_set_level(const char *tag, esp_log_level_t level)
{
    esp_log_level_set(tag ? tag : "*", level);
}

//* Sink Management
esp_err_t sys_log_register_sink(sys_log_sink_fn sink)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    s_sink = sink;
    ESP_LOGI(TAG, "Remote sink %s", sink ? "registered" : "cleared");
    return ESP_OK;
}

void sys_log_unregister_sink(void)
{
    s_sink = NULL;
}
