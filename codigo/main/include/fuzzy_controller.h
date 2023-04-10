#include "esp_system.h"


int run_fuzzy_step(int sensor_reading, int ambient_reading, bool accumulate);
void set_fuzzy_target(int _target);
void fuzzy_init(int _last_read, int _max_power);
