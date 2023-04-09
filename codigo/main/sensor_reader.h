#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "esp_timer.h"


void sensor_init();
int get_reading(int index);

bool reading_command(char* message);
void reading_command_help(char prefix);

bool temperature_command(char* message);
void temperature_command_help(char prefix);

int get_temperature(int index);
void sensors_task();

