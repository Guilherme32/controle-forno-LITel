#include "controller.h"
#include "helpers.h"

// Internal definitions ---------------------------------------------------------------------------

#define INTERRUPT_PIN 13
#define INTERRUPT_PIN_FLAG (1ULL << INTERRUPT_PIN)

#define OUTPUT_PIN_0 4
#define OUTPUT_PIN_1 5
#define STEADY_PIN 15
#define OUTPUT_PINS_FLAG   ( (1ULL << OUTPUT_PIN_0) \
                           | (1ULL << OUTPUT_PIN_1) \
                           | (1ULL << STEADY_PIN))

#define PERIOD 120
#define MAX_TARGET 140
#define MAX_TARGET_READING ((MAX_TARGET * 1023) / 200)
#define STEADY_LIMIT 4
#define STEADY_TIME 60

#define CONTROLLER_TAG "CONTROLLER"


// Global variables declaration -------------------------------------------------------------------

static int power_ratio[2];
static int power_counter;
static int cycles;

static bool control;
static int target;
static bool steady;
static int steady_count;

static unsigned long int last_time;        // In microseconds
static int last_sensor_read;

int (*read_sensor)();
int (*read_ambient)();


// Static functions declaration -------------------------------------------------------------------

static void IRAM_ATTR update_ratio();
static void IRAM_ATTR send_power();
static void IRAM_ATTR check_steady_state();
static void IRAM_ATTR set_pins();
static void check_counter_limits();


// Static functions -------------------------------------------------------------------------------

static void IRAM_ATTR send_power()
{
    unsigned long int new_time = esp_timer_get_time();
    unsigned int time_diff = new_time - last_time;

    if (time_diff < 4000) {                 // Debounce
        return;
    }
    last_time = new_time;

    if (power_counter > 0) {                // Actual power control
        power_counter -= power_ratio[1];
        set_pins(1);
    } else {
        power_counter += power_ratio[0];
        set_pins(0);
    }

    if ((cycles % PERIOD) == 0) {           // Steady state checking
        check_steady_state();
    }

    if (cycles >= (30 * PERIOD)) {          // Updates the ratio every 30 modulation PERIODs
        cycles = 0;
        update_ratio();
    }

    cycles ++;
}

static void IRAM_ATTR update_ratio()
{
    if (control) {
        int accumulate = steady && (abs(read_sensor() - target) > STEADY_LIMIT);
        unsigned int power = run_fuzzy_step(
            read_sensor(),
            read_ambient(),
            accumulate);

        power_ratio[0] = power;
        power_ratio[1] = PERIOD - power;
    }
}

static void IRAM_ATTR check_steady_state()
{
    if (abs(read_sensor() - last_sensor_read) <= STEADY_LIMIT) {
        steady_count ++;
    } else {
        last_sensor_read = read_sensor();
        steady_count = 0;
    }

    if (steady_count >= STEADY_TIME) {
        steady = true;
    } else {
        steady = false;
    }

    gpio_set_level(STEADY_PIN, steady);
}

static void IRAM_ATTR set_pins(int level)
{
    // At the moment doesn't support individual load power modulation
    gpio_set_level(OUTPUT_PIN_0, level);
    gpio_set_level(OUTPUT_PIN_1, level);
}

static void check_counter_limits()
{
    power_counter = power_counter >  power_ratio[0]?  power_ratio[0] : power_counter;
    power_counter = power_counter < -power_ratio[1]? -power_ratio[1] : power_counter;
}


// Public functions -------------------------------------------------------------------------------

void set_ratio(int cycles_on)
{
    if ((cycles_on > PERIOD) || (cycles_on < 0)) {
        return;
    }

    power_ratio[0] = cycles_on;
    power_ratio[1] = PERIOD - cycles_on;
    check_counter_limits();
    control = false;
}

void set_target(int _target)
{
    if (_target > MAX_TARGET_READING) {
        ESP_LOGE(CONTROLLER_TAG, "Tried to set a target too high (%d/ %d)",
            _target, MAX_TARGET_READING);
        return;
    }

    target = _target;
    set_fuzzy_target(target);
    control = true;
    update_ratio();
    check_counter_limits();
}

void controller_shutdown()
{
    control = false;
    power_counter = 0;
    power_ratio[0] = 0;
    power_ratio[1] = PERIOD;
    set_pins(0);
}

ControllerInfo controller_info()
{
    ControllerInfo out = {
        .steady = steady,
        .power_ratio = {power_ratio[0], power_ratio[1]},
        .control = control,
        .target = target
    };

    return out;
}

bool set_ratio_command(char* message)
{
    if (strlen(message) != 7) {
        return false;
    }
    if (strncmp(message, "setp", 4) != 0) {
        return false;
    }

    if (!is_number(&message[4], 3)) {
        printf("\nInvalid input. XXX must be a number with three digits (use ");
        printf("zero padding if needed)\n");
        return true;
    }

    int cycles_on = atoi(&(message[4]));
    if ((cycles_on < 0) || (cycles_on > PERIOD)) {
        printf("\nInvalid value. XXX must be between 0 and max period %s",
               "(120 default)\n");
        return true;
    }
    
    set_ratio(cycles_on);
    printf("\nRatio set successfully\n");
    return true;
}

void set_ratio_command_help(char prefix)
{
    printf("%csetpXXX       Sets the number of conducting cycles per period to XXX.\n", prefix);
    printf("               Within [000, 120] by default\n");
}

bool set_target_command(char* message)
{
    if (strlen(message) != 9) {
        return false;
    }
    if (strncmp(message, "sett", 4) != 0) {
        return false;
    }

    if (!is_number(&message[4], 3) || !is_number(&message[8], 1)) {
        printf("\nInvalid input. XXX.X must be a real number following the format, ");
        printf("as 050.1, or 100.0\n");
        return true;
    }

    float target = atof(&(message[4]));
    if (target > MAX_TARGET) {
        printf("\nInvalid value. The max target temperature is %d.0ºC\n", MAX_TARGET);
        return true;
    }

    set_target(target * 1023 / 200);
    printf("\nTarget temperature set successfully\n");
    return true;
}

void set_target_command_help(char prefix)
{
    printf("%csettXXX.X     Sets the target temperature to XXX.XºC.\n", prefix);
    printf("               Within [000.0, %d.0] by default\n", MAX_TARGET);
}

void controller_init(int (*_read_sensor)(), int (*_read_ambient)())
{
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INTERRUPT_PIN, send_power, NULL);

    gpio_config_t gpio_cfg;                                // Init output and stability pins
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_cfg.pin_bit_mask = OUTPUT_PINS_FLAG;
    gpio_cfg.pull_down_en = 0;
    gpio_cfg.pull_up_en = 0;
    gpio_config(&gpio_cfg);

    gpio_set_level(OUTPUT_PIN_0, 0);
    gpio_set_level(OUTPUT_PIN_1, 0);
    gpio_set_level(STEADY_PIN, 0);

    read_sensor = _read_sensor;                            // Init the reading functions
    read_ambient = _read_ambient;

    power_ratio[0] = 0;                                    // Init internal variables
    power_ratio[1] = PERIOD;
    power_counter = 0;
    cycles = 0;
    control = false;
    target = 0;
    last_time = esp_timer_get_time();
    last_sensor_read = read_sensor();
    steady_count = 0;
    steady = false;

    gpio_config_t intr_cfg;                                // Init interrupt
    intr_cfg.intr_type = GPIO_INTR_POSEDGE;
    intr_cfg.pin_bit_mask = INTERRUPT_PIN_FLAG;
    intr_cfg.mode = GPIO_MODE_INPUT;
    intr_cfg.pull_up_en = 0;
    intr_cfg.pull_down_en = 0;
    gpio_config(&intr_cfg);

    fuzzy_init(last_sensor_read, PERIOD);

    ESP_LOGI(CONTROLLER_TAG, "Initiated the controller");
}