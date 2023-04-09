/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#include "wifi.h"
#include "server.h"
#include "storage_spiffs.h"
#include "sensor_reader.h"
#include "controller.h"
#include "serial_comm.h"

int readings_placeholder() {        // TODO placeholder
    return 0;
}

void app_main()
{
    // Some system info print ---------------------------------------------------------------------
    print_header();
    ESP_LOGI("STARTUP", "Starting up the system");

    // Modules init -------------------------------------------------------------------------------
    sensor_init();
    wifi_init();
    spiffs_init();
    server_init(10);
    controller_init(readings_placeholder, readings_placeholder);
    serial_comm_init();

    // Linking modules ----------------------------------------------------------------------------
    add_command(netinfo_command, netinfo_command_help);
    add_command(set_ratio_command, set_ratio_command_help);
    add_command(set_target_command, set_target_command_help);
    add_command(reading_command, reading_command_help);
    add_command(temperature_command, temperature_command_help);

    // Starting modules tasks -------------------------------------------------
    xTaskCreate(wifi_led_task, "wifi_led", configMINIMAL_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(serial_comm_task, "serial", 2*configMINIMAL_STACK_SIZE, NULL, 10, NULL);
}
