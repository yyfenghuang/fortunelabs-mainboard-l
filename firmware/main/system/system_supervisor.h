/**
 *
 * @brief System health supervisor: task watchdog + heartbeat telemetry.
 * Bundles three system-health concerns that naturally belong together:
 *   1. Task Watchdog (TWDT) — tasks subscribe and must feed periodically;
 *      a stuck task is detected within the configured timeout.
 *   2. Panic/recovery — on TWDT expiry the ESP-IDF panic handler runs
 *      (backtrace + reboot). This is delegated, not re-implemented, so the
 *      crash backtrace is preserved.
 *   3. Heartbeat — a supervisor task periodically publishes uptime and heap
 *      stats (as JSON) to a remote sink, proving the scheduler is alive.
 * Constraint: a watched task must call feed() more often than wdt_timeout_ms,
 * otherwise it trips the watchdog. The supervisor task feeds itself
 * automatically at a safe sub-timeout cadence.
 */

#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Internal sizing
#define SYSTEM_SUP_DEVICE_ID_MAX 32
#define SYSTEM_SUP_HEARTBEAT 160

    //* Heartbeat publish callback
    /**
     * @brief Callback invoked by the supervisor task to ship a heartbeat payload to a remote transport
     *
     * @param json Null terminated JSON hartbeat payload
     * @param len
     *
     * @return - ESP_OK: Payload accepted by the transport
     * @return = ESP_FAIL: / Spesific error
     */
    typedef esp_err_t (*sys_health_publish_fn)(const char *json, size_t len);

    //* Supervisor config
    /**
     * @brief Config fot the watchdog and heartbeat subsystem
     *
     * @param wdt_timeout_ms  Watchdog timeout per subscribed task
     * @param trigger_panic If true, a TWDT expiry triggers the panic
     * @param heartbeat_period_ms  Interval between heartbeat publishes
     * @param publish Sink for heartbeat payloads (NULL disables
     * @param device_id Identifier embedded in each heartbeat
     *
     * @return - Void: This data structure not return a valuee
     */
    typedef struct
    {
        uint32_t wdt_timeout_ms;
        bool trigger_panic;
        uint32_t heartbeat_period_ms;
        sys_health_publish_fn publish;
        const char *device_id;
    } sys_supervisor_config_t;

    //* Lifecycle
    /**
     * @brief Init the task watchdog
     *
     * @param cfg  Pointer to a populated sys_supervisor_config_t.
     *
     * @return - ESP_OK: Watchdog configured and heartbeat parameters stored.
     * @return - ESP_ERR_INVALID_ARG: cfg is NULL or wdt_timeout_ms is zero.
     * @return - Specific esp_err_t: TWDT init/reconfigure failure.
     */
    esp_err_t sys_supervisor_init(const sys_supervisor_config_t *cfg);

    //* Per-task watchdog membership
    /**
     * @brief Subcribe the calling task to the watchdog
     *
     * @return - ESP_OK: Calling task is now watched.
     * @return - Specific esp_err_t: TWDT subscription failure (e.g. not initialized).
     */
    esp_err_t sys_supervisor_task_register(void);
    /**
     * @brief Feed (reset) the wathcdog for the calling task
     *
     * @return - ESP_OK: Watchdog reset for the calling task.
     * @return - Specific esp_err_t: Calling task is not subscribed.
     */
    esp_err_t sys_supervisor_task_feed(void);
    /**
     * @brief Unsubscribe the calling task from watchdog
     *
     * @return - ESP_OK: calling task removed from monitoring
     * @return - Specific esp_err_t: Calling task was not subscribed.
     */
    esp_err_t sys_supervisor_task_unregister(void);

    //* Supervisor task
    /**
     * @brief heartbeat task: self-feeds the watchdog and publish health
     *
     * @param pvParameters - Unused FreeRTOS task argument
     *
     * @return void : This task doen't return
     */
    void task_supervisor(void *pvParameters);
}