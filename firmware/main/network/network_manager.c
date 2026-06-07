#include "network_manager.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include "system/system_config.h"

static const char *TAG = "network_manager";

// Bridge for external access to actuator queue (for command handling in MQTT event)
extern QueueHandle_t g_queue_actuator; 

// Internal state
static system_config_t s_network_cfg;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static SemaphoreHandle_t s_status_mutex = NULL;
static bool s_wifi_connected = false;
static bool s_mqtt_connected = false;

//Topic storage for publish/subscribe
static char s_topic_telemetry[64];
static char s_topic_command[64];
static char s_topic_health[64];

// WIFI event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG, "WiFi started, connecting to AP...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        xSemaphoreTake(s_status_mutex, portMAX_DELAY);
        s_wifi_connected = false;
        s_mqtt_connected = false; // MQTT also considered disconnected if WiFi is down
        xSemaphoreGive(s_status_mutex);
        ESP_LOGW(TAG, "Wifi Disconnected, attempting to reconnect...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Successfully got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xSemaphoreTake(s_status_mutex, portMAX_DELAY);
        s_wifi_connected = true;
        xSemaphoreGive(s_status_mutex);

        // If wifi is connected, start MQTT connection
        if (s_mqtt_client){
            esp_mqtt_client_start(s_mqtt_client);
        }
    }
}

// MQTT Event handler
static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data){
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT client connected to broker: %s", s_network_cfg.broker_uri);
            xSemaphoreTake(s_status_mutex, portMAX_DELAY);
            s_mqtt_connected = true;
            xSemaphoreGive(s_status_mutex);

            // Subscribe to command topic upon successful connection
            int msg_id = esp_mqtt_client_subscribe(s_mqtt_client, s_topic_command, 1);
            ESP_LOGI(TAG, "Subscribed to command topic, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT client disconnected from broker");
            xSemaphoreTake(s_status_mutex, portMAX_DELAY);
            s_mqtt_connected = false;
            xSemaphoreGive(s_status_mutex);
            break;
        case MQTT_EVENT_DATA:
        //Event control handler
            ESP_LOGI(TAG, "Inbound Command Received!");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);

            // If string is "ON", send true to actuator queue
            if (strncmp(event->data, "ON", event->data_len) == 0) {
            bool state = true;
            xQueueSend(g_queue_actuator, &state, 0);
            ESP_LOGI(TAG, "Remote Control Action: Turned Actuator ON");
        }
            // If string is "OFF", send false to actuator queue
            else if (strncmp(event->data, "OFF", event->data_len) == 0) {
            bool state = false;
            xQueueSend(g_queue_actuator, &state, 0);
            ESP_LOGI(TAG, "Remote Control Action: Turned Actuator OFF");
        }
            break;
        default:
            break;
    }
}

// Telemery Task (Publish dummy sensor)
static void task_network_telemetry(void *pvParameters){
    (void)pvParameters;
    uint32_t tx_count = 0;
    char payload[128]; // Buffer for JSON payload

    ESP_LOGI(TAG, "Telemetry task started. 5s loop to publish dummy sensor data.");

    while(1){
        bool ready = false; // false until MQTT connection is up
        xSemaphoreTake(s_status_mutex, portMAX_DELAY);
        ready = s_mqtt_connected;
        xSemaphoreGive(s_status_mutex);

        if (ready){
            // Dummy data simulation
            float dummy_voltage = 1.5f + ((float)(tx_count % 10)) / 0.1f; // Simulate voltage between 1.5V to 2.5V

            snprintf(payload, sizeof(payload), "{\"seq\":\"%lu\",\"sensor_v\":%.2f,\"status\":\"OK\"}", tx_count++, dummy_voltage); // JSON payload with sequence number and dummy voltage reading

            int msg_id = esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, payload, 0, 1, 0);
            if (msg_id != -1){
                ESP_LOGI(TAG, "Telemetry published successfully: (msg_id: %d) %s", msg_id, payload);
            } else {
                ESP_LOGE(TAG, "Failed to publish telemetry data");
            }
        }
        else{
            ESP_LOGD(TAG, "MQTT not ready, skipping telemetry publish...");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Publish every 5 seconds

    }
}

// API Lifecycle 
esp_err_t network_manager_init(const system_config_t *cfg){
    if (cfg == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    // Copy config to internal state
    memcpy(&s_network_cfg, cfg, sizeof(system_config_t));

    // Create mutex for status protection
    s_status_mutex = xSemaphoreCreateMutex();
    if (s_status_mutex == NULL)
        return ESP_ERR_NO_MEM;

    // 1. Initialize TCP/IP stack and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 2. Setup wifi with parameters from NVS config
    // FIX: Nama variabel diubah dari wifi_config_t menjadi wifi_init_cfg agar tidak menabrak tipe data asli
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    // Register event handlers for WiFi and IP events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Sekarang compiler tahu kalau wifi_config_t di bawah ini adalah TIPE DATA, bukan variabel
    wifi_config_t wifi_sta_config = {0};
    strncpy((char *)wifi_sta_config.sta.ssid, s_network_cfg.wifi_ssid, sizeof(wifi_sta_config.sta.ssid));
    strncpy((char *)wifi_sta_config.sta.password, s_network_cfg.wifi_pass, sizeof(wifi_sta_config.sta.password));
    wifi_sta_config.sta.threshold.rssi = -127; // Accept all RSSI levels
    wifi_sta_config.sta.pmf_cfg.capable = true; // Enable Protected Management Frames

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    // Set explicit topics for telemetry and command for Fortune Labs Mainboard L
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "fortunelabs/device/%s/telemetry", s_network_cfg.device_id);
    snprintf(s_topic_command, sizeof(s_topic_command), "fortunelabs/device/%s/command", s_network_cfg.device_id);
    snprintf(s_topic_health, sizeof(s_topic_health), "fortunelabs/device/%s/health", s_network_cfg.device_id);
    ESP_LOGI(TAG, "Configured MQTT topics: Telemetry='%s', Command='%s'", s_topic_telemetry, s_topic_command); // Debug log for topic configuration

    // 3. Setup MQTT client config
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = s_network_cfg.broker_uri,
        .session.disable_clean_session = false,
        .network.reconnect_timeout_ms = 5000, // Reconnect after 5s if connection is lost
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if(s_mqtt_client == NULL){
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // Register MQTT event handler
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_LOGI(TAG, "Network manager initialized successfully. Target AP: '%s'", s_network_cfg.wifi_ssid);
    return ESP_OK;
}

esp_err_t network_manager_start(void){
    // Run wifi physical default driver
    ESP_ERROR_CHECK(esp_wifi_start());

    // Run telemetry task
    BaseType_t ok = xTaskCreate(task_network_telemetry, "task_network_telemetry", 4096, NULL, 3, NULL);
    if (ok != pdPASS){
        ESP_LOGE(TAG, "Failed to create telemetry task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}   

bool network_manager_is_connected(void){
    bool connected = false;
    if (s_status_mutex != NULL){
        xSemaphoreTake(s_status_mutex, portMAX_DELAY);
        connected = s_mqtt_connected;
        xSemaphoreGive(s_status_mutex);
    }
    return connected;
}

esp_err_t network_manager_publish_health(const char *json_payload, size_t length)
{
    // Check if MQTT client is ready before attempting to publish health data
    if (s_mqtt_client == NULL || !s_mqtt_connected) {
        // Balikan error jika network belum siap (supervisor akan tetap cetak lokal)
        return ESP_ERR_INVALID_STATE; 
    }

    // Publish health data using QoS 1 to ensure delivery to the broker
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, s_topic_health, json_payload, (int)length, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE("network_manager", "Failed to publish health metrics to MQTT");
        return ESP_FAIL;
    }

    return ESP_OK;
}