#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include <math.h>

#include <sys/unistd.h>
#include "esp_spiffs.h"

#include "wifi.h"
#include "sensor_reader.h"
#include "controller.h"

httpd_handle_t server_init(int task_prio);

