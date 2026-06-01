/**
 * Configuration store for the mainboard (NVS-backed).
 */

#pragma once

#include "esp_err.h"
#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

//* Field Capacities
#define SYSTEM_CFG_SSID_LEN 33
#define SYSTEM_CFG_PASS_LEN 64
#define SYSTEM_CFG_URI_LEN 128
#define SYSTEM_CFG_ID_LEN 32

    //* Configuration snapshot
    /**
     * @brief Resolve configuration values handed to the rest of stystem
     *
     * @param wifi_ssid Target access point SSID
     * @param wifi_pass Passkey for the target access point.
     * @param broker_uri MQTT broker endpoint
     * @param device_id Stable per-device identifier used in MQTT topics.
     *
     * @return - void : This data structure does not return a value.
     */
    typedef struct
    {
        char wifi_ssid[SYSTEM_CFG_SSID_LEN];
        char wifi_pass[SYSTEM_CFG_PASS_LEN];
        char broker_uri[SYSTEM_CFG_URI_LEN];
        char device_id[SYSTEM_CFG_ID_LEN];
    } sys_config_t;

    //* System Config Lifecycle
    /**
     * @brief Open the configuration NVS namespace and prepare the store.
     *
     * @return - ESP_OK: Namespace opened and store ready.
     * @return - ESP_FAIL / Specific esp_err_t: NVS not initialize
     */
    esp_err_t system_config_init(void);
    /**
     * @brief Resolve all configuration values into the caller's snapshot.
     *
     * @return - ESP_OK: Snapshot fully populated.
     * @return - ESP_ERR_INVALID_ARG: out is NULL.
     * @return - ESP_ERR_INVALID_STATE: sys_config_init() was not called first.
     * @return - Specific esp_err_t: NVS read failure on a present key.
     */
    esp_err_t system_config_load(sys_config_t *out);

    //* Runtime Provisioning
    /**
     * @brief Persist WiFi credentials to NVS, overriding the Kconfig default.
     *
     * @param ssid  Null-terminated SSID (≤ 32 chars).
     * @param pass Null-terminated passphrase (≤ 63 chars).
     *
     * @return - ESP_OK: Both keys written and committed.
     * @return - ESP_ERR_INVALID_ARG: ssid is NULL or exceeds capacity.
     * @return - ESP_ERR_INVALID_STATE: store not initialized.
     * @return - Specific esp_err_t: NVS write/commit failure.
     */
    esp_err_t system_config_set_wifi(const char *ssid, const char *pass);

    /**
     * @brief Persist the MQTT broker URI to NVS
     *
     * @param uri Null-terminated broker URI(<=127 chars)
     *
     * @return - ESP_OK: Key written and committed.
     * @return - ESP_ERR_INVALID_ARG: uri is NULL or exceeds capacity.
     * @return - ESP_ERR_INVALID_STATE: store not initialized.
     * @return - Specific esp_err_t: NVS write/commit failure.
     */
    esp_err_t system_config_set_broker(const char *uri);

    /**
     * @brief Copy the resolved devide identifier into the caller's buffer
     *
     * @param buf Destination buffer for null-terminated ID.
     * @param len Capacity of buf in byte
     *
     * @return - ESP_OK: ID copied successfully.
     * @return - ESP_ERR_INVALID_ARG: buf is NULL or len is zero.
     * @return - ESP_ERR_INVALID_STATE: store not initialized.
     */
    esp_err_t system_config_get_device_id(char *buf, size_t len);

    /**
     * @brief Erase all stored config, revert to Kconfig defaults
     *
     * @return - ESP_OK: Namespace erased and committed.
     * @return - ESP_ERR_INVALID_STATE: store not initialized.
     * @return - Specific esp_err_t: NVS erase/commit failure.
     */
    esp_err_t system_config_reset(void);

#ifdef __cplusplus
}
#endif