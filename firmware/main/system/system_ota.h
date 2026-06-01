/** *
 * ARCHITECTURAL_NOTE: FUTURE REFACTORING TO ADVANCED STEPPED OTA PATTERN
 *
 * Currently, this function uses the monolithic blocking call `esp_https_ota()`.
 * While clean and sufficient for simple deployments, it should be refactored
 * to the Advanced OTA pattern (`esp_https_ota_begin -> perform -> finish`)
 * to achieve a professional industrial-grade implementation:
 *  1. Real-Time UI/UX Feedback (Integration with write_display.c)
 * The monolithic call completely blinds the user interface during the
 * download process. By shifting to an iterative loop with `esp_https_ota_perform()`,
 * we can intercept the transfer state and update the display in real-time.
 * Example implementation:
 * int read_bytes = esp_https_ota_get_image_len_read(https_ota_handle);
 * 2. Mitigating FreeRTOS Task Starvation (Critical Telemetry Protection)
 * Network operations are highly blocking and unpredictable. If this task
 * hogging CPU time coincides with critical time-sensitive tasks—such as
 * high-frequency air regulator sensor sampling or hardware interrupts—the
 * monolithic call can starve those tasks, leading to watchdog resets or data loss.
 * An iterative loop allows us to inject structured `vTaskDelay()` periods
 * between packet processing chunks, smoothly yielding CPU cycles back to
 * the FreeRTOS scheduler so the rest of the mainboard stays active and responsive.
 */

/**
 * @brief Over-the-air firmware update with rollback protection.
 */
#pragma once

#include "esp_err.h"
#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //* OTA Configuration
    /**
     * @brief Parameters for a single OTA update attempt
     *
     * @param url Firmware image url
     * @param cert_pem Server certificate in PEM format for TLS
     * @param timeout_ms Per-request network timeout in ms
     *  @param reboot_on_success If true, reboot into the new image immediately
     * @param skip_cert_check If true, skip TLS common-name verification
     *
     * @return - void : This data structure doesnot return a value
     */
    typedef struct
    {
        const char *url;
        const char *cert_pem;
        uint32_t timeout_ms;
        bool reboot_on_success;
        bool skip_cert_check;
    } system_ota_config_t;

    //* Update operation
    /**
     * @brief Download, verify, and flash firmware image
     *
     * @param cfg Pointer to a populated system_ota_config_t
     *
     * @return - ESP_OK: Image flashed and boot partition set
     * @return - ESP_ERR_INVALID_ARG: cfg or cfg->url is NULL.
     * @return - Specific esp_err_t: Network, validation, or flash error; the running image is left untouched.
     */
    esp_err_t system_ota_perform(const system_ota_config_t *cfg);

    //* Rollback management
    /**
     * @brief Report wether the runing image is awaiting confirmation
     *
     * @return - true: Running image is in PENDING_VERIFY state
     * @return - false: Image already confirmed, or rollback is not enable
     */
    bool system_ota_pending_verify(void);

    /**
     * @brief Confirm the running image is healthy and cancel pending rollback
     *
     * @return - ESP_OK: Image marked valid; rollback cancelled.
     * @return - Specific esp_err_t: State update failure.
     */
    esp_err_t system_ota_mark_valid(void);

    //* Diagnostic
    /**
     * @brief Copy the running application's version string into a buffer.
     *
     * @param buf Destination buffer for the null-terminated version.
     * @param len Capacity of buf in bytes (32 is sufficient).
     *
     * @return - ESP_OK: Version copied successfully.
     * @return - ESP_ERR_INVALID_ARG: buf is NULL or len is zero.
     */
    esp_err_t system_ota_get_running_version(char *buf, size_t len);

#ifdef __cplusplus
}
#endif