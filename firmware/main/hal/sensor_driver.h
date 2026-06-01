/**
 * @brief Any sensor eg.potentiometer, ADS1115, SHT31, MQ-137, or future
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
    /**
     * @brief Configuration structure for initializing a sensor driver instance.
     *
     * @param bus          Pointer to the initialized I2C bus configuration structure.
     * @param i2c_addr     The 7-bit I2C slave address of the target sensor device.
     * @param channel      Default hardware channel or sub-sensor pin configuration.
     * @param extra        Generic pointer to a custom, driver-specific configuration.
     *
     * @return - void: This data structure does not return a value.
     */
    typedef struct
    {
        i2c_bus_t *bus;
        uint8_t i2c_addr;
        uint8_t channel;
        void *extra;
    } sensor_config_t;

    //* Reading sensor output
    /**
     * @brief Configuration structure for initializing a sensor driver instance.
     *
     * @param bus          Pointer to the initialized I2C bus configuration structure.
     * @param i2c_addr     The 7-bit I2C slave address of the target sensor device.
     * @param channel      Default hardware channel or sub-sensor pin configuration.
     * @param extra        Generic pointer to a custom, driver-specific configuration.
     *
     * @return - void: This data structure does not return a value.
     */
    typedef struct
    {
        uint8_t channel;
        float value;
        uint32_t timestamp_ms;
    } sensor_reading_t;

    //* Driver interface
    /**
     * @brief Hardware Abstraction Layer (HAL) interface contract for sensor drivers.
     *
     * @param cfg          Pointer to the sensor_config_t structure.
     * @param out          Pointer to the sensor_reading_t structure where telemetry data will be stored.
     *
     *
     * @return - ESP_OK: Operation completed successfully (returned by init and read).
     * @return - ESP_FAIL / Specific esp_err_t: Communication error, timeout, or sensor read failure.
     * @return - void: No return value provided (returned by deinit).
     */
    typedef struct
    {
        const char *name;

        /**
         * @brief Init the sensor hardware
         */
        esp_err_t (*init)(const sensor_config_t *cfg);

        /**
         * @brief Sensor read sample once before the loop start
         */
        esp_err_t (*read)(sensor_reading_t *out);

        /**
         * @brief Release hardware resource (optional, NULLABLE)
         */
        void (*deinit)(void);
    } sensor_driver_t;

#ifdef __cplusplus
}
#endif