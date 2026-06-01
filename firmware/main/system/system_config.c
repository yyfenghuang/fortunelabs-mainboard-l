#include "system/system_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_mac.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "system_config";

// NVS Namespace + keys
#define CFG_NAMESPACE "flab_cfg"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"
#define KEY_BROKER_URI "broker_uri"
#define KEY_DEVICE_ID "device_id"

// Compile time defaurlt (Resolved from Kconfig; do not set if empty)
#ifndef CONFIG_FLAB_WIFI_SSID
#define CONFIG_FLAB_WIFI_SSID ""
#endif
#ifndef CONFIG_FLAB_WIFI_PASS
#define CONFIG_FLAB_WIFI_PASS ""
#endif
#ifndef CONFIG_FLAB_BROKER_URI
#define CONFIG_FLAB_BROKER_URI ""
#endif

static nvs_handle_t s_handle = 0;
static bool s_initialized = false;

//* Internal Helpers
// Read string key from NVS
static esp_err_t load_str(const char *key, const char *def, char *out, size_t out_sz)
{
    size_t len = out_sz;
    esp_err_t ret = nvs_get_str(s_handle, key, out, &len);

    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        strlcpy(out, def ? def : "", out_sz);
        return ESP_OK;
    }
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "nvs_get_str(%s) failed: %s", key, esp_err_to_name(ret));
        strlcpy(out, def ? def : "", out_sz);
    }
    return ret;
}
// Generate Stable device ID from factory MAC
static void derive_device_id(char *out, size_t out_sz)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out, out_sz, "flab-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

//* Lifecycle

esp_err_t system_config_init(void)
{
    esp_err_t ret = nvs_open(CFG_NAMESPACE, NVS_READWRITE, &s_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open(%s) failed : %s", CFG_NAMESPACE, esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Config store ready (namespace = %s)", CFG_NAMESPACE);
    return ESP_OK;
}

esp_err_t system_config_load(sys_config_t *out)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    memset(out, 0, sizeof(sys_config_t));

    load_str(KEY_WIFI_SSID, CONFIG_FLAB_WIFI_SSID, out->wifi_ssid, sizeof(out->wifi_ssid));
    load_str(KEY_WIFI_PASS, CONFIG_FLAB_WIFI_PASS, out->wifi_pass, sizeof(out->wifi_pass));
    load_str(KEY_BROKER_URI, CONFIG_FLAB_BROKER_URI, out->broker_uri, sizeof(out->broker_uri));

    // device_id : NVS -> (no KConfig default) -> derive from MAC
    load_str(KEY_DEVICE_ID, "", out->device_id, sizeof(out->device_id));
    if (out->device_id[0] == '\0')
        derive_device_id(out->device_id, sizeof(out->device_id));

    ESP_LOGI(TAG, "Loaded config: ssid='%s' broker='%s' id='%s'", out->wifi_ssid, out->broker_uri, out->device_id);
    return ESP_OK;
}

//* Runtime provisioning

esp_err_t system_config_set_wifi(const char *ssid, const char *pass)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;
    if (ssid == NULL || strlen(ssid) >= SYSTEM_CFG_SSID_LEN)
        return ESP_ERR_INVALID_ARG;
    if (pass != NULL && strlen(pass) >= SYSTEM_CFG_PASS_LEN)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_set_str(s_handle, KEY_WIFI_SSID, ssid);
    if (ret != ESP_OK)
        return ret;

    ret = nvs_set_str(s_handle, KEY_WIFI_PASS, pass ? pass : "");
    if (ret != ESP_OK)
        return ret;

    ret = nvs_commit(s_handle);
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "WIFI credentials presisted (ssid='%s')", ssid);
    return ret;
}

esp_err_t system_config_set_broker(const char *uri)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;
    if (uri == NULL || strlen(uri) >= SYSTEM_CFG_URI_LEN)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret = nvs_set_str(s_handle, KEY_BROKER_URI, uri);
    if (ret == ESP_OK)
        return ret;

    ret = nvs_commit(s_handle);
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "Broker URI persisted ('%s')", uri);
    return ret;
}

esp_err_t system_config_get_device_id(char *buf, size_t len)
{
    if (buf == NULL || len == 0)
        return ESP_ERR_INVALID_ARG;
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = load_str(KEY_DEVICE_ID, "", buf, len);
    if (ret == ESP_OK && buf[0] == '\0')
        derive_device_id(buf, len);
    return ret;
}

esp_err_t system_config_reset(void)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    esp_err_t ret = nvs_erase_all(s_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_erase_all failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(s_handle);
    if (ret == ESP_OK)
        ESP_LOGW(TAG, "Config erased — reverting to compile-time defaults");
    return ret;
}