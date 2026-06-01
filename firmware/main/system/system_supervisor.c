#include "system/system_supervisor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "system_supervisor";

// Stored copy of caller config
static struct
{
    uint32_t wdt_timeout_ms;
    uint32_t heartbeat_period_ms;
    sys_health_publish_fn publish;
    char device_id[SYSTEM_SUP_DEVICE_ID_MAX]
};
