#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Common & HAL Contracts
#include "common/app_types.h"
#include "bus/i2c_bus.h"
#include "hal/sensor_driver.h"
#include "hal/output_driver.h"
#include "hal/display_driver.h"

// Concrete Drivers
#include "drivers/sensor/sensor_dummy.h"
#include "drivers/output/actuator_dummy.h"
#include "drivers/ic/ssd1306.h"

// System services
#include "system/system_config.h"
#include "system/system_log.h"
#include "system/system_ota.h"
#include "system/system_supervisor.h"


// I2c Bus Configuration
#define MAIN_I2C_SDA_PIN 18
#define MAIN_I2C_SCL_PIN 19
#define MAIN_I2C_PORT I2C_NUM_0

static const char *TAG = "main";

// Storage allocations for global resources
QueueHandle_t g_queue_display;
QueueHandle_t g_queue_actuator;
static i2c_bus_t g_i2c_bus;

// Task prototypes for RTOS 
extern void task_sensor(void *pvParameters);
extern void task_actuator(void *pvParameters);
extern void task_display(void *pvParameters);

void app_main(void)
{
    ESP_LOGI(TAG, "FortuneLabs Mainboard PoC - booting...");

    // 1. Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Init System Logging Facade
    ESP_ERROR_CHECK(system_log_init(ESP_LOG_INFO)); // Set default log level to INFO
    ESP_LOGI(TAG, "FortuneLabs Mainboard PoC - Booting ...");

    // 3. Init & Load System Configuration 
    ESP_ERROR_CHECK(system_config_init());
    system_config_t sys_cfg;
    ESP_ERROR_CHECK(system_config_load(&sys_cfg)); // Load config from NVS to RAM

    // 4. OTA Rollback Protection Check (if applicable)
    if (system_ota_pending_verify()){
        ESP_LOGI(TAG, "New Firmware booted successfully. Marking OTA as verified.");
        system_ota_mark_valid(); 
    }

    // 5. Init I2C master 
    i2c_bus_config_t bus_cfg = {
        .sda_pin = MAIN_I2C_SDA_PIN,
        .scl_pin = MAIN_I2C_SCL_PIN,
        .port = MAIN_I2C_PORT,
        .clk_hz = 100000 // 100 kHz Fast Mode
    };
    ESP_LOGI(TAG, "Initializing I2C Master Bus...");
    ESP_ERROR_CHECK(i2c_bus_init(&g_i2c_bus, &bus_cfg));

    // 6. Inter-task alloc for communication via FreeRTOS Queues
    g_queue_display = xQueueCreate(10, sizeof(display_msg_t)); // using struct for display messages (row + text)
    g_queue_actuator = xQueueCreate(5, sizeof(bool));          // using bool for simple ON/OFF control

    if (g_queue_display == NULL || g_queue_actuator == NULL)
    {
        ESP_LOGE(TAG, "Queue creation failed!");
        esp_restart();
    }

    // 7. Init all hardware drivers (Sensor, Actuator, Display) with their respective configs
    ESP_LOGI(TAG, "Initializing hardware drivers...");

    // Init Potentiometer Dummy
    sensor_config_t sensor_cfg = {.bus = NULL};
    ESP_ERROR_CHECK(sensor_dummy_driver.init(&sensor_cfg));

    // Init Onboard LED (Actuator)
    output_config_t actuator_cfg = {.bus = NULL, .num_channels = 1};
    ESP_ERROR_CHECK(actuator_dummy_driver.init(&actuator_cfg));

    // Init Physical SSD1306 OLED via I2C Bus
    display_config_t display_cfg = {
        .bus = &g_i2c_bus,
        .i2c_addr = 0x3C, // Alamat I2C standard SSD1306
        .width = 128,
        .height = 64};
    ESP_ERROR_CHECK(ssd1306_driver.init(&display_cfg));

    // 8. Init System Supervisor Task to monitor system health and perform watchdog resets if necessary
    system_supervisor_config_t supervisor_cfg = {
        .wdt_timeout_ms = 10000, // 10 detik timeout watchdog
        .trigger_panic = true,   
        .heartbeat_period_ms = 5000, // Publish heartbeat setiap 5 detik
        .publish = NULL, // Placeholder for health publish function (MQTT publish in real)
        .device_id = sys_cfg.device_id // Using device ID from system 
    };
    ESP_ERROR_CHECK(system_supervisor_init(&supervisor_cfg));

    

    // 9. Spawn Tasks RTOS for Sensor Reading, Actuator Control, and Display Update
    //Spawn supervisor task here 
    xTaskCreate(task_supervisor, "task_sys_sup", 3072, NULL, 6, NULL); // Highest priority for system supervisor

    xTaskCreate(task_sensor, "task_sensor", 3072, NULL, 5, NULL); // Task sensor have higher priority to ensure responsive reading
    xTaskCreate(task_actuator, "task_actuator", 2048, NULL, 5, NULL);
    xTaskCreate(task_display, "task_display", 3072, NULL, 4, NULL);

    ESP_LOGI(TAG, "All tasks spawned successfully. System is running.");
}