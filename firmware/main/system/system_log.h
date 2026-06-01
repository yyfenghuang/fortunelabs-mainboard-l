/**
 * @brief Logging facade over ESP-IDF esp_log with an optional remote sink.
 * This module deliberately does NOT introduce a parallel logging macro
 * system — application code keeps using ESP_LOGE / ESP_LOGW / ESP_LOGI /
 * ESP_LOGD as usual. sys_log adds two capabilities on top:
 */

#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

//* Forwarder buffer sizing
#define SYSTEM_LOG_LINE_MAX 160   // per-line buffer
#define SYSTEM_LOG_QUEUE_DEPTH 16 // pueued lines before drop-on-full

    //* Log sink callback
    /** @brief Callback invoked for each formatted log line, for forwarding to
     * a remote transport
     *
     * @param line         Null-terminated formatted log line, including the ESP-IDF level/tag/timestamp prefix.
     * @param len          Length of the line in bytes, excluding the terminating null.
     *
     * @return - void: This callback does not return a value.
     */
    typedef void (*system_log_sink_fn)(const char *line, size_t len);

    //* Logging Lifecycle
    /**
     * @brief Init logging: set the global level and install the sink
     * forwarder (queue + task + vprintf hook).
     *
     * @param default_level Global minimum level applied to all tags ("*").
     *
     * @return - ESP_OK: Logging facade ready.
     * @return - ESP_ERR_NO_MEM: Queue or forwarder task allocation failed.
     */
    esp_err_t system_log_init(esp_log_level_t default_level);

    //* Level Control
    /**
     * @brief Set the minimum log level for a specific tag at runtime.
     * Thin wrapper over esp_log_level_set(); pass "*" to affect all tags.
     *
     * @param tag Log tag to adjust, or "*" for the global level.
     * @param level Minimum level to emit for that tag.
     *
     * @return - void: This function does not return a value.
     */
    void system_log_set_level(const char *tag, esp_log_level_t level);

    //* Sink management
    /**
     * @brief Register (or replace) the remote log sink and start forwarding.
     *
     * @param sink Callback receiving each formatted line. Passing NULL is equivalent to sys_log_unregister_sink().
     *
     * @return - ESP_OK: Sink registered; forwarding active.
     * @return - ESP_ERR_INVALID_STATE: sys_log_init() was not called first.
     */
    esp_err_t system_log_register_sink(system_log_sink_fn sink);

    /**
     * @brief Stop forwarding to the remote sink. Serial output is unaffected.
     *
     * @return void: This function does not return a value.
     */
    void system_log_unregister_sink(void);

#ifdef __cplusplus
}
#endif