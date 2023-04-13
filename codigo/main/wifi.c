#include "wifi.h"


// Definitions ------------------------------------------------------------------------------------

#define WIFI_TAG "WIFI"

#define AP_SSID "forno-litel"
#define AP_PASSWORD "787Cu7kg"
#define AP_DEFAULT_IP "192.168.4.1"

#define STA_SSID "Internet do Japao"
#define STA_PASSWORD "lslbt78322"
#define STA_MAX_RETRY 10

#define STA_CONNECT_BIT BIT0
#define STA_FAIL_BIT BIT1

#define STATUS_PIN 2
#define STATUS_PIN_FLAG (1ULL << STATUS_PIN)


// Global variables declaration -------------------------------------------------------------------

static int sta_retries = 0;
static char sta_ip[20] = "";


// Static functions declaration -------------------------------------------------------------------

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
static void connect_sta();
static char* info_str(NetInfo info);


// Static functions definition --------------------------------------------------------------------

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    // sta events handling ----------------------------------------------------
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(WIFI_TAG, "WiFi station initialized. Attempting connection");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        sta_ip[0] = '\0';

        if (sta_retries < STA_MAX_RETRY) {
            esp_wifi_connect();
            sta_retries ++;
            ESP_LOGI(WIFI_TAG,
                "WiFi station connection failed, retrying (%d/%d)",
                sta_retries, STA_MAX_RETRY);
        } else {
            ESP_LOGI(WIFI_TAG,
                "WiFi station connection failed, no longer retrying");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "WiFi station connected successfully");
        ESP_LOGI(WIFI_TAG, "Station IP: %s", ip4addr_ntoa(&event->ip_info.ip));

        sta_retries = 0;
        sprintf(sta_ip, "%s", ip4addr_ntoa(&event->ip_info.ip));
    }
}

static void connect_sta()
{
    sta_retries = STA_MAX_RETRY;
    esp_wifi_disconnect();
    sta_retries = 0;
    esp_wifi_connect();
}

static char info_out[50 + 2*NET_CRED_MAX_LEN] = "";
static char* info_str(NetInfo info)
{
    sprintf(info_out,
        "SSID: %s, password: %s, ip: %s",
        info.ssid,
        info.password,
        strlen(info.ip) > 1 ? info.ip : "disconnected");

    return info_out;
}


// Public functions definition --------------------------------------------------------------------

bool update_config(char* ssid, char* password)
{
    wifi_config_t sta_config = {};

    sprintf((char*)sta_config.sta.ssid, "%s", ssid);
    sprintf((char*)sta_config.sta.password, "%s", password);

    if (strlen(password) != 0) {
        sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    esp_wifi_stop();

    esp_err_t err = esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config);
    if (err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Failed to set sta config (%s)", esp_err_to_name(err));
        return false;
    }

    esp_wifi_start();
    connect_sta();

    return true;
}

NetInfo sta_info()
{
    NetInfo out = {};
    sprintf(out.ip, "%s", sta_ip);

    wifi_config_t sta_config;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &sta_config);
    sprintf(out.ssid, "%s", sta_config.sta.ssid);
    sprintf(out.password, "%s", sta_config.sta.password);

    return out;
}

NetInfo ap_info()
{
    NetInfo out = {};

    sprintf(out.ip, "%s", AP_DEFAULT_IP);
    sprintf(out.ssid, "%s", AP_SSID);
    sprintf(out.password, "%s", AP_PASSWORD);

    return out;
}

bool netinfo_command(char* message)
{
    //netinfo
    if (strlen(message) != 7) {
        return false;
    }

    if (strncmp(message, "netinfo", 7) != 0) {
        return false;
    }

    printf("\nNetinfo: \n");
    printf("   > Station: %s\n", info_str(sta_info()));
    printf("   > Soft-AP: %s\n", info_str(ap_info()));

    return true;
}

void netinfo_command_help(char prefix)
{
    printf("%cnetinfo       Displays the wifi connection information\n", prefix);
}

void wifi_led_task()
{
    ESP_LOGI(WIFI_TAG, "Started Wifi led task");
    int off_count = 0;
    bool status_level = 0;

    while (1) {
        if (strlen(sta_ip) <= 1) {
            if (sta_retries < STA_MAX_RETRY) {        // Trying to connect - blink
                off_count = 0;
                status_level = !status_level;
                gpio_set_level(STATUS_PIN, status_level);
                vTaskDelay(200/portTICK_PERIOD_MS);

            } else {                                    // Unconnected - off
                if (off_count ++ >= 60) {                    // if off for a minute, try again
                    connect_sta();
                }
                status_level = 0;                            // led off
                gpio_set_level(STATUS_PIN, status_level);
                vTaskDelay(1000/portTICK_PERIOD_MS);

            }
        } else {                                        // Connected - on
            off_count = 0;
            status_level = 1;
            gpio_set_level(STATUS_PIN, status_level);
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }
}

void wifi_init()
{
    ESP_LOGI(WIFI_TAG, "Initializing WiFi...");

    esp_err_t err = nvs_flash_init();                               // Init nvs
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Means nvs was truncated, and must be erased and restarted
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    tcpip_adapter_init();                                   // Init the wifi
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, NULL));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASSWORD,
            .max_connection = 5,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen(AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "WiFi access point initialized. SSID:%s password:%s",
             AP_SSID, AP_PASSWORD);

    gpio_config_t gpio_cfg;                        // Init the status pin
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_cfg.pin_bit_mask = STATUS_PIN_FLAG;
    gpio_cfg.pull_down_en = 0;
    gpio_cfg.pull_up_en = 0;
    gpio_config(&gpio_cfg);
}

