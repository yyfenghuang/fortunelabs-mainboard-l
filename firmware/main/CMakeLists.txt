idf_component_register(
    SRCS
        "main.c"
        "bus/i2c_bus.c"
        "system/system_config.c"
        "system/system_log.c"   
        "system/system_supervisor.c"
        "system/system_ota.c"
        
        # --- FreeRTOS Orchestrator Tasks ---
        "tasks/task_sensor.c"
        "tasks/task_actuator.c"
        "tasks/task_display.c"
        
        # --- Hardware Drivers (HAL Implementations) ---
        "drivers/sensor/sensor_dummy.c"
        "drivers/output/actuator_dummy.c"
        "drivers/ic/ssd1306.c"

    INCLUDE_DIRS 
        "."
        ".."
        
    REQUIRES
        # ssd1306 <-- Di-comment karena kita sudah pakai driver native di drivers/ic/ssd1306.c
        nvs_flash
        driver
        esp_adc
        esp_wifi
        esp_event
        esp_netif
        mqtt
        
    PRIV_REQUIRES 
        esp_https_ota 
        esp_http_client
        app_update
        
)