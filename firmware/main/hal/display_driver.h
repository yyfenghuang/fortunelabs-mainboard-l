/**
 * @brief abstraction layer (HAL) contract for display driver
 *
 * Any display interface use this contract
 *
 * Design note: display_show_text() uses row-based addressing
 * (like a terminal), not pixel coordinates. This keeps the
 * interface simple and works across very different displays.
 * For pixel-level control, use display_raw() (future extension).
 */

#pragma once

#include "esp_err.h"
#include "bus/i2c_bus.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //* Display Configuration
    /**
     * @brief Configuration structure for initializing a display driver instance.
     *
     * @param bus          Pointer to the initialized I2C bus configuration structure.
     * @param i2c_addr     The 7-bit I2C slave address of the target display device.
     * @param width        The physical horizontal resolution or character capacity.
     * @param height       The physical vertical resolution or row capacity.
     * @param extra        Generic pointer to a custom, driver-specific configuration.
     *
     * @return void: This data structure does not return a value.
     */
    typedef struct
    {
        i2c_bus_t *bus;
        uint8_t i2c_addr;
        uint16_t width;
        uint16_t height;
        void *extra;
    } display_config_t;

    //* Driver interface (vTable)
    /**
     * @brief Hardware Abstraction Layer (HAL) interface contract for display drivers.
     *
     * @param cfg          Pointer to the display_config_t structure.
     * @param row          The zero-indexed target row number for text placement.
     * @param text         Null-terminated string to be displayed.
     * @param level        The brightness intensity scale ranging from 0 to 255.
     *
     *
     * @return - ESP_OK: Operation completed successfully.
     * @return - ESP_FAIL / Specific esp_err_t: Hardware communication or configuration error.
     * @return - void: No return value provided (returned by deinit).
     */
    typedef struct
    {
        const char *name;

        /**
         * @brief Init display hardware
         */
        esp_err_t (*init)(const display_config_t *cfg);
        /**
         * @brief Clear entire display
         */
        esp_err_t (*clear)(void);
        /**
         * @brief show line of text at given row
         */
        esp_err_t (*show_text)(uint8_t row, const char *text);
        /**
         * @brief Set display brightnesss (0-255)
         */
        esp_err_t (*set_brightness)(uint8_t level);
        /**
         * @brief Release display resource (nullable)
         */
        void (*deinit)(void);
    } display_driver_t;
#ifdef __cplusplus
}
#endif