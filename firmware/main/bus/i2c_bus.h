#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Device Limitation
#define I2C_BUS_MAX_DEVICES 8
#define I2C_BUS_TIMEOUT_MS 100

    // Device entry registry
    typedef struct
    {
        uint8_t addr; // 7 bit I2C address
        i2c_master_dev_handle_t handle;
        const char *label;
        bool active;
    } i2c_device_entry_t;

    // Bus Device Configuration
    typedef struct
    {
        int sda_pin;
        int scl_pin;
        i2c_port_t port;
        uint32_t clk_hz;
    } i2c_bus_config_t;

    // Bus Instance
    typedef struct
    {
        i2c_master_bus_handle_t bus_handle;
        SemaphoreHandle_t mutex;
        i2c_device_entry_t devices[I2C_BUS_MAX_DEVICES];
        uint8_t device_count;
        uint32_t error_count;
        bool initialized;
    } i2c_bus_t;

    // * Lifecycle
    // Init the I2CBus
    esp_err_t i2c_bus_init(i2c_bus_t *bus, const i2c_bus_config_t *cfg);
    // De-initialize bus and release all resoruce
    esp_err_t i2c_bus_deinit(i2c_bus_t *bus);

    // * Device Management
    // Register a device on the bus
    /**
     * @param addr      7-bit I2C address (e.g. 0x48 for ADS1115)
     * @param scl_hz    Clock speed for this device
     * @param label     Human-readable name for logging
     * @param out       Receives the device handle for subsequent R/W calls
     */
    esp_err_t i2c_bus_add_device(i2c_bus_t *bus, uint8_t addr, uint32_t scl_hz, const char *label, i2c_master_dev_handle_t *out);

    //*Data Transfer
    // Write data
    esp_err_t i2c_bus_write(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *data, size_t len);
    // Read data
    esp_err_t i2c_bus_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, uint8_t *buf, size_t len);
    // Register read pattern
    esp_err_t i2c_bus_write_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *wr_data, size_t wr_len, uint8_t *rd_buf, size_t rd_len);

    //* Diagnostic
    // Scan bus for all responding devices (0x03–0x77).
    esp_err_t i2c_bus_scan(i2c_bus_t *bus);
    // Publish lifetime error count, publish in health heartbeat
    uint32_t i2c_bus_get_error_count(const i2c_bus_t *bus);
    // Get num of registered device
    uint8_t i2c_bus_get_device_count(const i2c_bus_t *bus);

#ifdef __cplusplus
}
#endif