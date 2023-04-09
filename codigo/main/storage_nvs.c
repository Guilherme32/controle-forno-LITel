#include "storage_nvs.h"

#define STORAGE_TAG "NVS"



bool save_str(char* key, char* value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(key, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) trying to open %s", 
            esp_err_to_name(err), key);
        return false;
    }

    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) trying to save %s", 
            esp_err_to_name(err), key);
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) commiting changes to %s",
            esp_err_to_name(err), key);
        return false;
    }

    nvs_close(handle);
    return true;
}

bool get_str(char* key, char* value, size_t len)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(key, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) trying to open %s", 
            esp_err_to_name(err), key);
        return false;
    }

    err = nvs_get_str(handle, key, value, &len);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) trying to get %s", 
            esp_err_to_name(err), key);
        return false;
    }

    nvs_close(handle);
    return true;
}

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Means nvs was truncated, and must be erased and restarted
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // opening nvs
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error (%s) opening NVS handle!\n",
            esp_err_to_name(err));
        return;
    }

    // reading variable
    int32_t start_counter = 1;
    err = nvs_get_i32(my_handle, "start_counter", &start_counter);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(STORAGE_TAG, "This device has been started %d times",
                start_counter);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(STORAGE_TAG,
                "It is the first time this device was initiated!");
            break;
        default:
            ESP_LOGE(STORAGE_TAG, "Error reading device starts: %s",
                esp_err_to_name(err));
    }

    // writing variable
    start_counter ++;
    err = nvs_set_i32(my_handle, "start_counter", start_counter);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error writing device starts: %s",
            esp_err_to_name(err));
    }
    
    // commiting
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(STORAGE_TAG, "Error commiting changes to nvs: %s",
            esp_err_to_name(err));
    }

    nvs_close(my_handle);

    ESP_LOGI(STORAGE_TAG, "Storage initialized successfully");
}
