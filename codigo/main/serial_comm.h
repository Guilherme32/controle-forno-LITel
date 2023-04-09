#include <string.h>
#include "FreeRTOS/freertos.h"
#include "esp_log.h"
#include "driver/uart.h"


void add_command(bool (*new_command)(char*), void(*command_help)(char));
void print_header();
void serial_comm_task();
void serial_comm_init();
