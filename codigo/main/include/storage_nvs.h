#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

void storage_init(void);
bool save_str(char* key, char* value);
bool get_str(char* key, char* value, size_t len);
