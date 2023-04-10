#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "fuzzy_controller.h"


#ifndef CONTROLLER_IMPORT
typedef struct ControllerInfo {
    bool steady;
    int power_ratio[2];
    bool control;
    int target;
} ControllerInfo;

#define CONTROLLER_IMPORT

#endif

void set_ratio(int cycles_on);
void set_target(int _target);
void controller_shutdown();
ControllerInfo controller_info();

bool set_ratio_command(char* command);
void set_ratio_command_help(char prefix);

bool set_target_command(char* command);
void set_target_command_help(char prefix);

void controller_init(int (*_read_sensor)(), int (*_read_ambient)());
