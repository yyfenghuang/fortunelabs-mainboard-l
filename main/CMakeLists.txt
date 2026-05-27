idf_component_register(
    SRCS
        "main.c"
        "read_sensor.c"
        "write_display.c"
        "write_actuator.c"
    INCLUDE_DIRS "."
    REQUIRES ssd1306 nvs_flash driver esp_adc
)