/**
 * @file 
 */

 #pragma once

 #include "bus/i2c_bus.h"
 #include "esp_err.h"
 #include <stdint.h>
 #include <stdbool.h>

 #ifdef __cplusplus
extern "C"{
    #endif

    // Register Map
    #define ADS1115_REG_CONVERSION 0x00
    #define ADS1115_REG_CONFIG 0x01
    #define ADS1115_REG_LO_THRESHOLD 0x02
    #define ADS1115_REG_HI_THRESHOLD 0x03

    //Register Reset Value
    #define ADS1115_CONFIG_RESET 0x8583U

    //Conversion Timing
    #define ADS1115_CONVERSION_MARGIN_MS 2U 

    //Enum mapping
    // I2C address selection
    typedef enum
    {
        ADS1115_ADDR_GND = 0x48,
        ADS1115_ADDR_VDD = 0x49,
        ADS1115_ADDR_SDA = 0x42,
        ADS1115_ADDR_SCL = 0x4B,
    } ads115_addr_t;

    //Input MUX channel selection
    typedef enum {
        ADS1115_CHANNEL_0 = 4, //0b100
        ADS1115_CHANNEL_1 = 5, //0b101
        ADS1115_CHANNEL_2 = 6, //0b110
        ADS1115_CHANNEL_3 = 7, //0b111
    } ads1115_channel_t;

    // Programmable Gain Amplifier (PGA)
    typedef enum {
        ADS1115_PGA_6_144V = 0, //0b000 -> ±6.144V FSR
        ADS1115_PGA_4_096V = 1, //0b001 -> ±4.096V FSR
        ADS1115_PGA_2_048V = 2, //0b010 -> ±2.048V FSR
        ADS1115_PGA_1_024V = 3, //0b011 -> ±1.024V FSR
        ADS1115_PGA_0_512V = 4, //0b100 -> ±0.512V FSR
        ADS1115_PGA_0_256V = 5, //0b101 -> ±0.256V FSR
    } ads1115_pga_t;

    //Output Data rate
    typedef enum{
        ADS1115_ADDR_8SPS = 0, //0b000 -> 8SPS, ~125ms conversion
        ADS1115_ADDR_16SPS = 1, //0b001 -> 8SPS, ~62.5ms conversion
        ADS1115_ADDR_32SPS = 2, //0b010 -> 8SPS, ~31.25ms conversion
        ADS1115_ADDR_64SPS = 3, //0b011 -> 8SPS, ~15.6ms conversion
        ADS1115_ADDR_128SPS = 4, //0b100 -> 8SPS, ~7.8ms conversion
        ADS1115_ADDR_250SPS = 5, //0b101 -> 8SPS, ~4.0ms conversion
        ADS1115_ADDR_475SPS = 6, //0b110 -> 8SPS, ~2.1ms conversion
        ADS1115_ADDR_860SPS = 7, //0b111 -> 8SPS, ~1.2ms conversion
    } ads1115_data_rate_t;

    //Per-channel config
    //This decision i make for mainboard flexibiliity, instead using global config
    typedef struct 
    {
        ads1115_channel_t channel;
        ads1115_pga_t pga;
        ads1115_rate_t data_rate;
    } ads1115_channel_config_t;

    //Driver instance
    typedef struct 
    {
        i2c_bus_t *bus;
        ads1115_addr_t addr;
        ads1115_channel_config_t channel_cfg[4];
        bool initialized;
    } ads1115_dev_t;

    //Driver init config
    typedef struct 
    {
        i2c_bus_t *bus;
        ads1115_addr_t addr;
        ads1115_channel_config_t channel_cfg[4];
    } ads1115_config_t;

    //Driver vTable
    typedef struct 
    {
        /**
         * Init
         */
        esp_err_t (*init)(ads_dev_t *dev, const ads1115_config_t *cfg);

        /**
         * Trigger single shot
         */
        esp_err_t (*read)(ads1115_dev_t *dev, ads1115_channel_config_t);
        /**
         * Write factory default value to config register
         */
        esp_err_t(*reset)(ads1115_dev_t *dev);
        /**
         * De-init all resource held by driver instance
         */
        void(*deinit)(ads1115_dev_t *dev);
    } ads1115_driver_t;
    
    //Concrete Driver Access
    /**
     * Return pointer into singleton concrete driver vTable
     */
    const ads1115_driver_t *ads1115_get_driver(void);

    #ifdef __cplusplus
}
#endif