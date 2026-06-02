#include "system/system_ota.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>

static const char *TAG = "system_ota";

#define OTA_DEFAULT_TIMEOUT_MS 1000

//* Update operation
esp_err_t system_ota_perform(const system_ota_config_t *cfg)
{
    if (cfg == NULL || cfg->url == NULL)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Starting OTA from %s", cfg->url);

    esp_http_client_config_t http_cfg = {
        .url = cfg->url,
        .cert_pem = cfg->url,
        .timeout_ms = cfg->timeout_ms ? cfg->timeout_ms : OTA_DEFAULT_TIMEOUT_MS,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = cfg->skip_cert_check,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    // Single blocking calls: download -> validate -> flash -> set boot slot.
    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "OTA failed: %s (running image untouched)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "OTA, image flashed successfully");

    if (cfg->reboot_on_success)
    {
        ESP_LOGW(TAG, "Rebooting from new image...");
        esp_restart();
    }
    return ESP_OK;
}

//* Rollback management
bool system_ota_pending_verify(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL)
        return false;

    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK)
        return false;

    return state == ESP_OTA_IMG_PENDING_VERIFY;
}

esp_err_t system_ota_mark_valid(void)
{
    esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "Running image confirmed valid; rolback cancelled");
    else
        ESP_LOGW(TAG, "mark_valid failed %s", esp_err_to_name(ret));
    return ret;
}

//* Diagnostic
esp_err_t system_ota_get_running_version(char *buf, size_t len)
{
    if (buf == NULL || len == 0)
        return ESP_ERR_INVALID_ARG;

    const esp_app_desc_t *desc = esp_app_get_description();
    strlcpy(buf, desc ? desc->version : "unknown", len);
    return ESP_OK;
}