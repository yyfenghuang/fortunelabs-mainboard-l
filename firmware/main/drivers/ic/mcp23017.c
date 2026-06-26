/**
 * @file mcp23017.c
 * @brief MCP23017 I2C GPIO Expander Driver Implementation
 */

#include "mcp23017.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mcp23017_driver";

/* ---------------------------- PRIVATE HELPER -------------------------- */
/**
 * @brief Guard: validates that the driver instance is initialized and ready.
 *
 * @param dev   Pointer to mcp23017_t context struct
 *
 * @return
 * - true  : dev is non-NULL and is_initialized is true
 * - false : dev is NULL or driver has not completed init
 */
static inline bool _mcp23017_is_ready(const mcp23017_t *dev) {
    return (dev != NULL && dev->is_initialized);
}

/**
 * @brief Write one byte to a single internal MCP23017 register over I2C.
 *
 * Constructs a 2-byte payload [reg, data] passed to i2c_bus_write().
 * Maps to MCP23017 write lifecycle: START → addr+W → reg → data → STOP.
 *
 * @param dev   Pointer to mcp23017_t context struct
 * @param reg   Target register address (IOCON.BANK = 0 map)
 * @param data  Byte value to write into the register
 *
 * @return
 * - ESP_OK   : Register written successfully
 * - ESP_FAIL : I2C transaction error (no ACK or bus fault)
 */
static esp_err_t _mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t data) {
    uint8_t   buf[2] = {reg, data};
    esp_err_t ret    = i2c_bus_write(dev->bus_handle, dev->dev_addr, buf, sizeof(buf));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: reg=0x%02X data=0x%02X err=0x%X", reg, data, ret);
    }
    return ret;
}

/**
 * @brief Read one byte from a single internal MCP23017 register over I2C.
 *
 * Issues a write (sets internal address pointer) then a repeated-start read.
 * Maps to MCP23017 read lifecycle:
 * START → addr+W → reg → REPEATED START → addr+R → data → NACK → STOP.
 *
 * @param dev       Pointer to mcp23017_t context struct
 * @param reg       Target register address (IOCON.BANK = 0 map)
 * @param out_data  Pointer to uint8_t that receives the register value
 *
 * @return
 * - ESP_OK   : Register read successfully, out_data populated
 * - ESP_FAIL : I2C transaction error
 */
static esp_err_t _mcp23017_read_reg(mcp23017_t *dev, uint8_t reg, uint8_t *out_data) {
    esp_err_t ret = i2c_bus_write_read(dev->bus_handle, dev->dev_addr, &reg, 1, out_data, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: reg=0x%02X err=0x%X", reg, ret);
    }
    return ret;
}

/* ---------------------------- DRIVER LIFECYCLE -------------------------- */
esp_err_t mcp23017_init(mcp23017_t *dev, mcp23017_config_t *config) {
    // [1] Validate
    if (dev == NULL || config == NULL || config->bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->address < MCP23017_ADDR_BASE || config->address > MCP23017_ADDR_MAX) {
        ESP_LOGE(TAG, "Invalid I2C address: 0x%02X (valid: 0x%02X-0x%02X)", config->address,
                 MCP23017_ADDR_BASE, MCP23017_ADDR_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    // [2] Tranfer config to context
    dev->bus_handle     = config->bus;
    dev->dev_addr       = config->address;
    dev->is_initialized = false;

    esp_err_t ret;

    // [3] Hardware init sequence
    // Step A: Force IOCON.BANK = 0, regardless of prior hardware state.
    ret = _mcp23017_write_reg(dev, 0x05, 0x00);
    if (ret != ESP_OK)
        return ret;
    ret = _mcp23017_write_reg(dev, MCP23017_REG_IOCON, 0x00);
    if (ret != ESP_OK)
        return ret;

    // Step B: Write I/O direction
    ret = _mcp23017_write_reg(dev, MCP23017_REG_IODIRA, config->dir_a);
    if (ret != ESP_OK)
        return ret;
    ret = _mcp23017_write_reg(dev, MCP23017_REG_IODIRB, config->dir_b);
    if (ret != ESP_OK)
        return ret;

    // Step C: Write pull-up configuration registers
    ret = _mcp23017_write_reg(dev, MCP23017_REG_GPPUA, config->pullup_a);
    if (ret != ESP_OK)
        return ret;

    ret = _mcp23017_write_reg(dev, MCP23017_REG_GPPUB, config->pullup_b);
    if (ret != ESP_OK)
        return ret;

    // Step D: Write output latches to known zero state
    ret = _mcp23017_write_reg(dev, MCP23017_REG_OLATA, MCP23017_DEFAULT_OLAT);
    if (ret != ESP_OK)
        return ret;

    ret = _mcp23017_write_reg(dev, MCP23017_REG_OLATB, MCP23017_DEFAULT_OLAT);
    if (ret != ESP_OK)
        return ret;

    // [4] Synchronize shadow registers to match hardware state written above
    dev->dir_a  = config->dir_a;
    dev->dir_b  = config->dir_b;
    dev->olat_a = MCP23017_DEFAULT_OLAT;
    dev->olat_b = MCP23017_DEFAULT_OLAT;

    // [5] Mark driver ready — only reachable if every hardware write above succeeded
    dev->is_initialized = true;

    ESP_LOGD(TAG, "Init OK: addr=0x%02X dir_a=0x%02X dir_b=0x%02X pullup_a=0x%02X pullup_b=0x%02X",
             dev->dev_addr, dev->dir_a, dev->dir_b, config->pullup_a, config->pullup_b);

    return ESP_OK;
}

void mcp23017_deinit(mcp23017_t *dev) {

    if (dev == NULL) {
        return;
    }
    memset(dev, 0, sizeof(mcp23017_t)); // Driver instance is inert and safe to reinitialize.
}

/* ---------------------------- PIN-LEVEL OPERATION -------------------------- */
esp_err_t mcp23017_set_pin_direction(mcp23017_t *dev, mcp23017_port_t port, uint8_t pin,
                                     mcp23017_direction_t direction) {
    if (!_mcp23017_is_ready(dev))
        return ESP_ERR_INVALID_STATE;
    if (port != MCP23017_PORT_A && port != MCP23017_PORT_B)
        return ESP_ERR_INVALID_ARG;
    if (pin > MCP23017_PIN_MAX)
        return ESP_ERR_INVALID_ARG;
    if (direction != MCP23017_DIR_INPUT && direction != MCP23017_DIR_OUTPUT)
        return ESP_ERR_INVALID_ARG;

    uint8_t *shadow = (port == MCP23017_PORT_A) ? &dev->dir_a : &dev->dir_b;
    uint8_t  reg    = (port == MCP23017_PORT_A) ? MCP23017_REG_IODIRA : MCP23017_REG_IODIRB;

    // RMW on shadow — no I2C read required
    if (direction == MCP23017_DIR_INPUT) {
        *shadow |= (uint8_t)(1u << pin); // set bit   → input
    } else {
        *shadow &= (uint8_t)~(1u << pin); // clear bit → output
    }

    return _mcp23017_write_reg(dev, reg, *shadow);
}

esp_err_t mcp23017_write_pin(mcp23017_t *dev, mcp23017_port_t port, uint8_t pin, bool value) {
    if (!_mcp23017_is_ready(dev))
        return ESP_ERR_INVALID_STATE;
    if (port != MCP23017_PORT_A && port != MCP23017_PORT_B)
        return ESP_ERR_INVALID_ARG;
    if (pin > MCP23017_PIN_MAX)
        return ESP_ERR_INVALID_ARG;

    uint8_t *shadow = (port == MCP23017_PORT_A) ? &dev->olat_a : &dev->olat_b;
    uint8_t  reg    = (port == MCP23017_PORT_A) ? MCP23017_REG_OLATA : MCP23017_REG_OLATB;

    // RMW on shadow — no I2C read required
    if (value) {
        *shadow |= (uint8_t)(1u << pin);
    } else {
        *shadow &= (uint8_t)~(1u << pin);
    }

    return _mcp23017_write_reg(dev, reg, *shadow);
}

esp_err_t mcp23017_read_pin(mcp23017_t *dev, mcp23017_port_t port, uint8_t pin, bool *out_value) {
    if (!_mcp23017_is_ready(dev))
        return ESP_ERR_INVALID_STATE;
    if (port != MCP23017_PORT_A && port != MCP23017_PORT_B)
        return ESP_ERR_INVALID_ARG;
    if (pin > MCP23017_PIN_MAX)
        return ESP_ERR_INVALID_ARG;
    if (out_value == NULL)
        return ESP_ERR_INVALID_ARG;

    uint8_t   reg = (port == MCP23017_PORT_A) ? MCP23017_REG_GPIOA : MCP23017_REG_GPIOB;
    uint8_t   raw = 0;
    esp_err_t ret = _mcp23017_read_reg(dev, reg, &raw);
    if (ret != ESP_OK)
        return ret;

    // Extract target bit — result is exactly 0 or 1, maps cleanly to bool
    *out_value = ((raw & (uint8_t)(1u << pin)) != 0u);
    return ESP_OK;
}

esp_err_t mcp23017_toggle_pin(mcp23017_t *dev, mcp23017_port_t port, uint8_t pin) {
    if (!_mcp23017_is_ready(dev))
        return ESP_ERR_INVALID_STATE;
    if (port != MCP23017_PORT_A && port != MCP23017_PORT_B)
        return ESP_ERR_INVALID_ARG;
    if (pin > MCP23017_PIN_MAX)
        return ESP_ERR_INVALID_ARG;

    uint8_t *shadow = (port == MCP23017_PORT_A) ? &dev->olat_a : &dev->olat_b;
    uint8_t  reg    = (port == MCP23017_PORT_A) ? MCP23017_REG_OLATA : MCP23017_REG_OLATB;

    // XOR on shadow — no I2C read required
    *shadow ^= (uint8_t)(1u << pin);

    return _mcp23017_write_reg(dev, reg, *shadow);
}