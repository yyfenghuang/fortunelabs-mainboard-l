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

// Drivers Konkret
#include "drivers/sensor/sensor_dummy.h"
#include "drivers/output/actuator_dummy.h"
#include "drivers/ic/ssd1306.h"

// Konfigurasi Pin I2C (Sesuaikan dengan pin physical board kamu)
#define MAIN_I2C_SDA_PIN 18
#define MAIN_I2C_SCL_PIN 19
#define MAIN_I2C_PORT I2C_NUM_0

static const char *TAG = "main";

// Alokasi Storage Object Global
QueueHandle_t g_queue_display;
QueueHandle_t g_queue_actuator;
static i2c_bus_t g_i2c_bus;

// Prototipe Fungsi Task (Bisa dipindah ke header task jika ada)
extern void task_sensor(void *pvParameters);
extern void task_actuator(void *pvParameters);
extern void task_display(void *pvParameters);

void app_main(void)
{
    ESP_LOGI(TAG, "FortuneLabs Mainboard PoC - booting...");

    // 1. Inisialisasi Non-Volatile Storage (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inisialisasi Shared Hardware Bus (I2C Master)
    i2c_bus_config_t bus_cfg = {
        .sda_pin = MAIN_I2C_SDA_PIN,
        .scl_pin = MAIN_I2C_SCL_PIN,
        .port = MAIN_I2C_PORT,
        .clk_hz = 100000 // 400 kHz Fast Mode
    };
    ESP_LOGI(TAG, "Initializing I2C Master Bus...");
    ESP_ERROR_CHECK(i2c_bus_init(&g_i2c_bus, &bus_cfg));

    // 3. Alokasi Inter-Task Queues dengan Tipe Data Baru
    g_queue_display = xQueueCreate(10, sizeof(display_msg_t)); // Menggunakan struct row text
    g_queue_actuator = xQueueCreate(5, sizeof(bool));          // Menggunakan command boolean

    if (g_queue_display == NULL || g_queue_actuator == NULL)
    {
        ESP_LOGE(TAG, "Queue creation failed!");
        esp_restart();
    }

    // 4. Inisialisasi Seluruh Driver via HAL Interface Contract
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

    // 5. Spawn FreeRTOS Tasks dengan Manajemen Prioritas & Stack yang Optimal
    // Task Sensor di-set prioritas sedikit lebih tinggi untuk stabilitas sampling rate
    xTaskCreate(task_sensor, "task_sensor", 3072, NULL, 5, NULL);
    xTaskCreate(task_actuator, "task_actuator", 2048, NULL, 5, NULL);
    xTaskCreate(task_display, "task_display", 3072, NULL, 4, NULL);

    ESP_LOGI(TAG, "All tasks spawned successfully. System is running.");
}