/**
 * Hardware abstraction layer (HAL) contract for outputs
 * from this mainboard.
 *
 * This prevents coupling between business logic
 * and hardware logic and provides a standard structure
 * for every output.
 *
 * Design note: channels are 0-indexed. The mapping from logical
 * channel number to physical pin/register is internal to the driver.
 * For example, output_relay driver might map:
 *   channel 0 → MCP23017 GPA0 → ULN2803A IN1 → Relay 1
 *   channel 1 → MCP23017 GPA1 → ULN2803A IN2 → Relay 2
 */

#pragma once

#include "esp_err.h"
#include "bus/i2c_bus.h"
#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //* Output Configuration
    /**
     * @brief Configuration structure for initializing an output driver instance.
     *
     * @param bus          Pointer to the shared I2C bus configuration (can be NULL if using native GPIOs).
     * @param i2c_addr     The 7-bit I2C slave address of the target expander device (e.g., 0x20 for MCP23017).
     * @param num_channels Total number of output pins or channels the driver is expected to manage.
     * @param extra        Generic pointer to a custom, driver-specific configuration.
     *
     * @return void: This data structure does not return a value.
     */
    typedef struct
    {
        i2c_bus_t *bus;
        uint8_t i2c_addr;
        uint8_t num_channels;
        void *extra;
    } output_config_t;

    //* Driver Interface (vTable)
    /**
     * @brief Hardware Abstraction Layer (HAL) interface contract for multi-channel output drivers.
     *
     * @param cfg          Pointer to the output_config_t structure.
     * @param channel      The zero-indexed hardware channel or pin number to modify or read.
     * @param state        Boolean state flag where true forces high/active and false forces low/inactive.
     * @param bitmask      An 8-bit map representation where Bit 0 corresponds to Channel 0.
     *
     * @return
     * - ESP_OK: Operation completed successfully (returned by init, set, get, and set_all).
     * - ESP_FAIL / Specific esp_err_t: Hardware communication error or invalid channel index.
     * - void: No return value provided (returned by deinit).
     */
    typedef struct
    {
        const char *name;
        uint8_t num_channels;

        /** @brief Init hardware peripheral, configure pin directions, and set default initial state to OFF */
        esp_err_t (*init)(const output_config_t *cfg);

        /** @brief Set a single channel output state to either high (ON) or low (OFF) */
        esp_err_t (*set)(uint8_t channel, bool state);

        /** @brief Read back the current latch or register state of a specific channel */
        esp_err_t (*get)(uint8_t channel, bool *state);

        /** @brief Update all available channels simultaneously using an 8-bit hardware state map */
        esp_err_t (*set_all)(uint8_t bitmask);

        /** @brief Release hardware resources and reset expander states (optional, NULLABLE) */
        void (*deinit)(void);
    } output_driver_t;

#ifdef __cplusplus
}
#endif