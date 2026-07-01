/**
 * @file i2c_bus.h
 * @brief Platform I2C bus abstraction layer.
 *
 * Shared platform infrastructure used by all I2C IC drivers (ADS1115,
 * MCP23017, etc). Accessed via direct function calls, not a vtable.
 *
 * Thread safety is enforced internally via a mutex held for the duration
 * of each transaction; callers do not need external locking.
 */

#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device Limitation
#define I2C_BUS_MAX_DEVICES 8
#define I2C_BUS_TIMEOUT_MS 100

// Device entry registry
typedef struct {
    uint8_t                 addr; // 7 bit I2C address
    i2c_master_dev_handle_t handle;
    const char             *label;
    bool                    active;
} i2c_device_entry_t;

// Bus Device Configuration
typedef struct {
    int        sda_pin;
    int        scl_pin;
    i2c_port_t port;
    uint32_t   clk_hz;
} i2c_bus_config_t;

// Bus Instance
typedef struct {
    i2c_master_bus_handle_t bus_handle;
    SemaphoreHandle_t       mutex;
    i2c_device_entry_t      devices[I2C_BUS_MAX_DEVICES];
    uint8_t                 device_count;
    uint32_t                error_count;
    bool                    initialized;
} i2c_bus_t;

/* --------------------------- LIFECYCLE ----------------------------*/
/**
 * @brief Bus lifecycle: init and de-init.
 *
 * init() creates the mutex, brings up the ESP-IDF I2C master bus, and
 * marks the instance ready. deinit() tears the bus down and releases
 * the mutex; the instance must not be reused without a fresh init().
 *
 * @param bus  Pointer to caller-allocated i2c_bus_t context struct
 * @param cfg  Pointer to i2c_bus_config_t (SDA/SCL pins, port, clock)
 *
 * @return
 * - ESP_OK              : Bus ready for use
 * - ESP_ERR_INVALID_ARG : bus or cfg is NULL
 * - ESP_ERR_NO_MEM      : Mutex allocation failed
 * - ESP_ERR_INVALID_STATE : deinit() called on a bus that was never initialized
 * - ESP_FAIL / Specific esp_err_t : Underlying i2c_new_master_bus() failure
 */
esp_err_t i2c_bus_init(i2c_bus_t *bus, const i2c_bus_config_t *cfg);
/** @brief De-initialize bus and release all resources held by the instance */
esp_err_t i2c_bus_deinit(i2c_bus_t *bus);

/* --------------------------- DEVICE MANAGEMENT ----------------------------*/
/**
 * @brief Register a downstream I2C device on this bus.
 *
 * Software-only registration against the ESP-IDF driver and the internal
 * device table. No I2C transaction occurs here, so a call can succeed
 * even if the device is not physically present on the bus.
 *
 * @param bus     Pointer to an already-initialized i2c_bus_t instance
 * @param addr    7-bit I2C address (e.g. 0x48 for ADS1115)
 * @param scl_hz  Clock speed for this device
 * @param label   Human-readable name for logging and diagnostics
 * @param out     Receives the device handle for subsequent R/W calls
 *
 * @return
 * - ESP_OK              : Device registered, out populated
 * - ESP_ERR_INVALID_ARG : bus not initialized, or out is NULL
 * - ESP_ERR_NO_MEM      : Device registry full (I2C_BUS_MAX_DEVICES reached)
 * - ESP_FAIL / Specific esp_err_t : Underlying i2c_master_bus_add_device() failure
 */
esp_err_t i2c_bus_add_device(i2c_bus_t *bus, uint8_t addr, uint32_t scl_hz, const char *label,
                             i2c_master_dev_handle_t *out);

/* --------------------------- DATA TRANSFER ----------------------------*/
/**
 * @brief Mutex-guarded I2C data transfer primitives.
 *
 * Each call acquires the bus mutex for the duration of the transaction
 * and releases it before returning, so callers never need external
 * locking. A failed mutex acquisition (contention beyond I2C_BUS_TIMEOUT_MS)
 * is reported the same as a transaction failure.
 *
 * @param bus      Pointer to an already-initialized i2c_bus_t instance
 * @param dev      Device handle obtained from i2c_bus_add_device()
 * @param data     Buffer to transmit
 * @param buf      Buffer to receive into
 * @param len      Length of data/buf in bytes
 * @param wr_data  Buffer to transmit before the repeated-start read
 * @param wr_len   Length of wr_data in bytes
 * @param rd_buf   Buffer to receive the repeated-start read into
 * @param rd_len   Length of rd_buf in bytes
 *
 * @return
 * - ESP_OK                : Transaction completed successfully
 * - ESP_ERR_INVALID_STATE : Bus not initialized
 * - ESP_ERR_TIMEOUT       : Mutex could not be acquired within I2C_BUS_TIMEOUT_MS
 * - ESP_FAIL / Specific esp_err_t : Underlying I2C transaction failure (no ACK, bus fault)
 */
esp_err_t i2c_bus_write(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *data,
                        size_t len);
/** @brief Read len bytes from dev into buf */
esp_err_t i2c_bus_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, uint8_t *buf, size_t len);
/** @brief Write wr_data then repeated-start read rd_len bytes into rd_buf (register read pattern)
 */
esp_err_t i2c_bus_write_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *wr_data,
                             size_t wr_len, uint8_t *rd_buf, size_t rd_len);

/* --------------------------- DIAGNOSTIC ----------------------------*/
/**
 * @brief Bus-level diagnostics: presence scan and lifetime counters.
 *
 * @param bus  Pointer to an already-initialized i2c_bus_t instance
 *
 * @return
 * - ESP_OK   : Scan completed (see logs for per-address ACK results)
 * - ESP_ERR_INVALID_STATE : Bus not initialized
 * - ESP_ERR_TIMEOUT       : Mutex could not be acquired within I2C_BUS_TIMEOUT_MS
 * - uint32_t : Lifetime transaction error count
 * - uint8_t  : Number of devices currently registered on the bus
 */
esp_err_t i2c_bus_scan(i2c_bus_t *bus);
/** @brief Publish lifetime error count, e.g. for a health heartbeat */
uint32_t i2c_bus_get_error_count(const i2c_bus_t *bus);
/** @brief Get number of devices currently registered on the bus */
uint8_t i2c_bus_get_device_count(const i2c_bus_t *bus);

#ifdef __cplusplus
}
#endif