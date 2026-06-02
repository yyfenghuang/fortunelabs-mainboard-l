idf_component_register(
    SRCS
        "main.c"
        # "read_sensor.c"
        # "write_display.c"
        # "write_actuator.c"
        # "comm.c"
        "bus/i2c_bus.c"
        "system/system_config.c"
        "system/system_log.c"   
        "system/system_supervisor.c"
        "system/system_ota.c"
    INCLUDE_DIRS 
    "."
    ".."
    REQUIRES
        ssd1306
        nvs_flash
        driver
        esp_adc
        esp_wifi
        esp_event
        esp_netif
        mqtt
    PRIV_REQUIRES esp_https_ota esp_http_client
)