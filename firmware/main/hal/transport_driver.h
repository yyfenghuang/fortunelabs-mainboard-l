/**
 * @brief abstraction layer (HAL) contract for communication
 * from this mainboard.
 *
 * Any communication interface use this contract
 *
 * Design note: transport doesn't use I²C bus (it uses WiFi/radio),
 * so config is different from sensor/display/output drivers.
 */

#pragma once

#include "esp_err.h"
#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //* Command callback
    /**
     * @brief Asynchronous callback function triggered when a control command is received from the remote server or broker.
     *
     * @param topic Endpoint the command came from
     * @param data Payuload in bytes
     * @param data_len Payload length
     *
     * @return void: This callback does not return any value.
     */
    typedef void(transport_cmd_cb_t)(const char *topic, const uint8_t *data, size_t data_len);

    //* Transport configuration
    /**
     * @brief Configuration structure for initializing a transport driver instance.
     * Contains the necessary connection credentials, device identifiers, and asynchronous
     * command callbacks, along with an open pointer for hardware-specific extensions.
     *
     * @param broker_uri   The target server address or endpoint URI
     * @param device_id    Unique identifier for this device, often derived from the hardware MAC address.
     * @param cmd_cb       Function pointer to the callback triggered when a remote command is received.
     * @param extra        Generic pointer to a custom, driver-specific configuration structure (can be NULL).
     *
     * @return void: As a configuration structure, it holds data and does not return a value.
     */
    typedef struct
    {
        const char *broker_uri;
        const char *device_id;
        transport_cmd_cb_t cmd_cb; // command callback
        void *extra;               // driver can extend their config
    } transport_config_t;

    //* Driver interface (vtable)
    /**
     * @brief Communication HAL interface contract (vtable) for transport drivers.
     * This structure defines the unified blueprint that any underlying communication protocol
     *
     * @param cfg          Pointer to the transport_config_t structure
     * @param topic        String identifier for the destination topic or endpoint
     * @param payload      The actual data content to be transmitted
     * @param len          The size or length of the payload in bytes
     *
     * @return
     * - ESP_OK: Operation completed successfully
     * - ESP_FAIL / Specific esp_err_t: Connection failed, timeout, or transmission error occurred.
     * - true / false: Boolean status indicating the current link connectivity
     * - void: No return value provided
     */
    typedef struct
    {
        const char *name;

        /** @brief Init transport, connect to broker or server */
        esp_err_t (*init)(const transport_config_t *cfg);
        /** @brief Publish telemetry payload to broker */
        esp_err_t (*publish)(const char *topic, const char *payload, size_t len);
        /** @brief Subscribe to command/control topic */
        esp_err_t (*subscribe)(const char *topic);
        /** @brief Check if transport connected and ready
         *  @return true if connected, false otherwise
         */
        bool (*is_connected)(void);
        /** @brief Disconnected from network, and release allocated resource */
        void (*deinit)(void);
    } transport_driver_t;
}
