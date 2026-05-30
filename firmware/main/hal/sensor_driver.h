/**
 * Any sensor eg.potentiometer, ADS1115, SHT31, MQ-137, or future
 * daughter board sensors implements this interface.
 *
 * The task layer (task_sensor.c) only depends on this contract,
 * never on a specific sensor IC. Swapping sensors = swapping the
 * driver pointer in main.c, zero changes to business logic.
 *
 * Design note: `value` is deliberately generic data type (float). What it
 * represents (voltage, temperature, ppm) depends on the driver.
 */
#pragma once

#include "esp_err.h"
#include "bus/i2c_bus.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Reading sensor output
    typedef struct
    {
        uint8_t channel;
        float value;
        uint32_t timestamp_ms;
    } sensor_reading_t;

    // Driver config init
    typedef struct
    {
        i2c_bus_t *bus;
        uint8_t i2c_addr;
        uint8_t channel;
        void *extra;
    } sensor_config_t;

    // Driver interface
    typedef struct
    {
        const char *name;

        /**
         * Init the sensor hardware
         * Just called once before read loop
         * The driver should register itself I2
         */
        esp_err_t (*init)(const sensor_config_t *cfg);

        /**
         * Then, sensor read sample once before the loop start (which loop?)
         * drivers should register itself on I2C bus here
         *
         * Return EPS_OK when success, error code when failure
         */
        esp_err_t (*read)(sensor_reading_t *out);

        /**
         * Release hardware resource
         * (optional, NULLABLE)
         */
        void (*deinit)(void);
    } sensor_driver_t;

#ifdef __cplusplus
}
#endif