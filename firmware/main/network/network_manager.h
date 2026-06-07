/**
 * @brief Resilient network STA and MQTT manager for ESP32, with auto-reconnect and event handling.
 */

 #pragma once

#include "esp_err.h"
#include "system/system_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief Initialize the network manager (TCP/IP, WiFi, MQTT Client)
 * @param config Pointer to the system configuration structure
 * @return ESP_OK if initialization is successful, otherwise an error code
 */
esp_err_t network_manager_init(const system_config_t *cfg);

/**
 * @brief Start connecting to Wifi and run telemetry tasks
 * @return ESP_OK if connection process started successfully, otherwise an error code
 */
esp_err_t network_manager_start(void);

/**
 * @brief Check if the network is currently connected to MQTT
 * @return true if connected, false otherwise
 */
bool network_manager_is_connected(void);

/**
 * @brief Public wrapper to publish health data to MQTT topic (for external use, e.g. from system supervisor)
 * @param json_payload JSON string payload to publish to health topic
 * @param length Length of the JSON payload string
 * @return ESP_OK if publish is successful, otherwise an error code
 */
esp_err_t network_manager_publish_health(const char *json_payload, size_t length);

#ifdef __cplusplus
}
#endif
