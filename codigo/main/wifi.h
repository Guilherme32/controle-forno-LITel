#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"


#ifndef WIFI_IMPORT

#define NET_CRED_MAX_LEN 64
typedef struct NetInfo {
  char ssid[NET_CRED_MAX_LEN];
  char password[NET_CRED_MAX_LEN];
  char ip[20];
} NetInfo;

#define WIFI_IMPORT

#endif

bool update_config(char* ssid, char* password);
NetInfo sta_info();
NetInfo ap_info();
bool netinfo_command(char* message);
void netinfo_command_help(char prefix);
void wifi_led_task();
void wifi_init();
