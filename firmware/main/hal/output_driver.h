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
    typedef struct
    {
        i2c_bus_t *bus;       /*Shared I2C bus (might null if use default ESP3 2 GPIO)*/
        uint8_t i2c_addr;     // Example 0x20 for MCP23017
        uint8_t num_channels; // Num of output that driver can manage
        void *extra;          // Driver extended config
    } output_config_t;

    //* Driver Interface (vTable)
    typedef struct
    {
        const char *name;
        uint8_t num_channels;

        // Init output hardware, make them output, set'em OFF
        esp_err_t (*init)(const output_config_t *cfg);
        // Set single channel ON (true) || OFF (false)
        esp_err_t (*set)(uint8_t channel, bool state);
        // Read back the current state
        esp_err_t (*get)(uint8_t channel, bool *state);
        // Set all channel via bitmask
        esp_err_t (*set_all)(uint8_t bitmask); // Bit 0 = channel 0
        // Release resource (optional, NULLable)
        void (*deinit)(void);
    } output_driver_t;

#ifdef __cplusplus
}
#endif