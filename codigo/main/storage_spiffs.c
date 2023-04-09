#include "storage_spiffs.h"

#define SPIFFS_TAG "SPIFFS"
#define TEST_SPIFFS 1


void spiffs_test(void)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(SPIFFS_TAG, "initiating SPIFFS test sequence");
    ESP_LOGI(SPIFFS_TAG, "Opening file");
    FILE* f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(SPIFFS_TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(SPIFFS_TAG, "Renaming file");
    if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
        ESP_LOGE(SPIFFS_TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(SPIFFS_TAG, "Reading file");
    f = fopen("/spiffs/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(SPIFFS_TAG, "Read from file: '%s'", line);
    ESP_LOGI(SPIFFS_TAG, "Finished SPIFFS test sequence");
}

void spiffs_init(void)
{
    ESP_LOGI(SPIFFS_TAG, "Initializing SPIFFS filesystem");

    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 15,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&config);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format spiffs filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_TAG, "Failed to find a SPIFFS partition");
        } else {
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS (%s)",
                esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to get SPIFFS partition information (%s)",
            esp_err_to_name(ret));
    } else {
        ESP_LOGI(SPIFFS_TAG, "SPIFFS partition size: total: %d, used: %d",
            total, used);
    }

    ESP_LOGI(SPIFFS_TAG, "SPIFFS filesystem initalized successfully");

    if (TEST_SPIFFS) {
        spiffs_test();
    }
}
