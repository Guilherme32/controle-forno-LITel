#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"


/**
* Inicializa (faz o mount) o sistema de arquivos do esp.
*/
void spiffs_init();
