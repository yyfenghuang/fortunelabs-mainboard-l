/**
 * @file i2c_bus.c
 * @brief Platform I2C Bus Abstraction Layer Implementation
 */

#include "bus/i2c_bus.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "i2c_bus";

/* ---------------------------- LIFECYCLE -------------------------- */
esp_err_t i2c_bus_init(i2c_bus_t *bus, const i2c_bus_config_t *cfg) {
    if (bus == NULL || cfg == NULL)
        return ESP_ERR_INVALID_ARG;

    memset(bus, 0, sizeof(i2c_bus_t));

    // Create mutex
    bus->mutex = xSemaphoreCreateMutex();
    if (bus->mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Configuring ESP IDF 5.x.x I2C master bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = cfg->port,
        .scl_io_num                   = cfg->scl_pin,
        .sda_io_num                   = cfg->sda_pin,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true, // ! True if doesnt have resistor
    };

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus->bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        vSemaphoreDelete(bus->mutex);
        return ret;
    }

    bus->initialized = true;
    ESP_LOGI(TAG, "Bus init OK, port=%d SDA=%d SCL=%d clk=%luHz", cfg->port, cfg->sda_pin,
             cfg->scl_pin, (unsigned long)cfg->clk_hz);

    return ESP_OK;
}

esp_err_t i2c_bus_deinit(i2c_bus_t *bus) {
    if (bus == NULL || !bus->initialized)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = i2c_del_master_bus(bus->bus_handle);
    if (bus->mutex)
        vSemaphoreDelete(bus->mutex);

    bus->initialized = false;
    ESP_LOGI(TAG, "Bus has been de-initialized");
    return ret;
}

/* ---------------------------- DEVICE MANAGEMENT -------------------------- */
esp_err_t i2c_bus_add_device(i2c_bus_t *bus, uint8_t addr, uint32_t scl_hz, const char *label,
                             i2c_master_dev_handle_t *out) {
    if (bus == NULL || !bus->initialized || out == NULL)
        return ESP_ERR_INVALID_ARG;
    if (bus->device_count >= I2C_BUS_MAX_DEVICES) {
        ESP_LOGE(TAG, "Device registry full (%d/%d)", bus->device_count, I2C_BUS_MAX_DEVICES);
        return ESP_ERR_NO_MEM;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = scl_hz,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus->bus_handle, &dev_cfg, out);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device 0x%02X (%s): %s", addr, label ? label : "?",
                 esp_err_to_name(ret));
        return ret;
    }

    // Register entry in the device table
    i2c_device_entry_t *entry = &bus->devices[bus->device_count];
    entry->addr               = addr;
    entry->handle             = *out;
    entry->label              = label;
    entry->active             = true;
    bus->device_count++;

    ESP_LOGI(TAG, "Device added: 0x%02X (%s), %lu Hz [%d/%d slots]", addr, label ? label : "?",
             (unsigned long)scl_hz, bus->device_count, I2C_BUS_MAX_DEVICES);

    return ESP_OK;
}

/* ---------------------------- DATA TRANSFER -------------------------- */
esp_err_t i2c_bus_write(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *data,
                        size_t len) {
    if (bus == NULL || !bus->initialized)
        return ESP_ERR_INVALID_STATE;

    if (!xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(I2C_BUS_TIMEOUT_MS))) {
        bus->error_count++;
        ESP_LOGE(TAG, "Bus mutex timeout");
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = i2c_master_transmit(dev, data, len, I2C_BUS_TIMEOUT_MS);
    xSemaphoreGive(bus->mutex);

    if (ret != ESP_OK) {
        bus->error_count++;
        ESP_LOGW(TAG, "Write failed %s (errors: %lu)", esp_err_to_name(ret),
                 (unsigned long)bus->error_count);
    }
    return ret;
}

esp_err_t i2c_bus_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, uint8_t *buf, size_t len) {
    if (bus == NULL || !bus->initialized)
        return ESP_ERR_INVALID_STATE;

    if (!xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(I2C_BUS_TIMEOUT_MS))) {
        bus->error_count++;
        ESP_LOGE(TAG, "Bus mutex timeout");
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = i2c_master_transmit(dev, buf, len, I2C_BUS_TIMEOUT_MS);
    xSemaphoreGive(bus->mutex);

    if (ret != ESP_OK) {
        bus->error_count++;
        ESP_LOGW(TAG, "Read failed: %s (errors: %lu)", esp_err_to_name(ret),
                 (unsigned long)bus->error_count);
    }
    return ret;
}

esp_err_t i2c_bus_write_read(i2c_bus_t *bus, i2c_master_dev_handle_t dev, const uint8_t *wr_data,
                             size_t wr_len, uint8_t *rd_buf, size_t rd_len) {
    if (bus == NULL || !bus->initialized)
        return ESP_ERR_INVALID_STATE;

    if (!xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(I2C_BUS_TIMEOUT_MS))) {
        bus->error_count++;
        ESP_LOGE(TAG, "Bus mutex timeout");
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret =
        i2c_master_transmit_receive(dev, wr_data, wr_len, rd_buf, rd_len, I2C_BUS_TIMEOUT_MS);
    xSemaphoreGive(bus->mutex);

    if (ret != ESP_OK) {
        bus->error_count++;
        ESP_LOGW(TAG, "Write-read failed: %s (errors: %lu)", esp_err_to_name(ret),
                 (unsigned long)bus->error_count);
    }
    return ret;
}

/* ---------------------------- DIAGNOSTIC -------------------------- */
esp_err_t i2c_bus_scan(i2c_bus_t *bus) {
    if (bus == NULL || !bus->initialized)
        return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "===== I2C Bus Scan =====");
    int found = 0;

    if (!xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(I2C_BUS_TIMEOUT_MS))) {
        bus->error_count++;
        ESP_LOGE(TAG, "Bus mutex timeout");
        return ESP_ERR_TIMEOUT;
    }
    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        // Send zero bytes to probe for a device ACK at this address
        esp_err_t ret = i2c_master_probe(bus->bus_handle, addr, I2C_BUS_TIMEOUT_MS);

        if (ret == ESP_OK) {
            const char *known = "unknown";
            for (int i = 0; i < bus->device_count; i++) {
                if (bus->devices[i].addr == addr && bus->devices[i].label) {
                    known = bus->devices[i].label;
                    break;
                }
            }
            ESP_LOGI(TAG, "0x%02X - ACK (%s)", addr, known);
            found++;
        }
    }

    xSemaphoreGive(bus->mutex);

    ESP_LOGI(TAG, "===== Scan Completed, %d Device(s) Found =====", found);
    return ESP_OK;
}

uint32_t i2c_bus_get_error_count(const i2c_bus_t *bus) { return bus ? bus->error_count : 0; }

uint8_t i2c_bus_get_device_count(const i2c_bus_t *bus) { return bus ? bus->device_count : 0; }