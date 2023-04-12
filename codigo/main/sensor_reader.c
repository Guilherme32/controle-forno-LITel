#include "sensor_reader.h"


// Definitions ------------------------------------------------------------------------------------

#define CONTROL_PIN_0 12
#define CONTROL_PIN_1 14
#define CONTROL_PIN_2 16
#define CONTROL_PINS_FLAG   ((1ULL << CONTROL_PIN_0) \
                           | (1ULL << CONTROL_PIN_1) \
                           | (1ULL << CONTROL_PIN_2))
#define READ_PERIOD 500
#define MULTISAMPLING 16
#define SENSOR_TAG "SENSOR_READER"

#define TEST_SENSORS_SPEED 0


// Global variables definition --------------------------------------------------------------------

int readings[8] = {0};


// Static functions declaration -------------------------------------------------------------------

static int reading_to_temp(int reading);
static void set_control_pins(int value);
static void read_temperatures();
static void test_speed();


// Static functions definition --------------------------------------------------------------------

static int reading_to_temp(int reading)
{
    // voltage = (reading * 2) / 1023
    // temp = voltage / 0.01
    return (reading * 2  * 100) / 1023;
}

static void set_control_pins(int value)
{
    gpio_set_level(CONTROL_PIN_0, (value >> 0) % 2);
    gpio_set_level(CONTROL_PIN_1, (value >> 1) % 2);
    gpio_set_level(CONTROL_PIN_2, (value >> 2) % 2);
}

static void read_temperatures()
{
    for (int i=0; i<8; i++) {
        set_control_pins(i);
        uint32_t mean = 0;
        uint16_t raw_reading;

        for (int j=0; j<MULTISAMPLING; j++) {
            adc_read(&raw_reading);
            mean += raw_reading;
        }
        mean /= MULTISAMPLING;

        readings[i] = mean;
    }
}

static void test_speed()
{
    uint16_t raw_reading;

    for (int i=0; i<10; i++) {
        uint32_t mean_reading = 0;
        unsigned int start_time = esp_timer_get_time();
    
        for (int j=0; j<1000; j++) {
            adc_read(&raw_reading);
            mean_reading += raw_reading;
        }
    
        mean_reading /= 1000;
        unsigned int end_time = esp_timer_get_time();

        printf("Read 1000 times (mean %u) in %u us\n", mean_reading, 
            end_time-start_time);
    }
}

// Public functions definition --------------------------------------------------------------------

int get_reading(int index)
{
    if (index >= 8 || index < 0) {
        return 0;
    }

    int reading = readings[index];

    return reading;
}

int get_temperature(int index)
{
    if (index >= 8 || index < 0) {
        return 0;
    }

    int reading = get_reading(index);
    return reading_to_temp(reading);
}

bool reading_command(char* message)
{
    if (strlen(message) != 7) {
        return false;
    }
    if (strncmp(message, "sensor", 6) != 0) {
        return false;
    }

    int index = (int)(message[6] - '0');
    if (index < 0 || index > 7) {
        printf("\nInvalid sensor index. The values possible are within [0,8)\n");
    } else {
        printf("\nReading on sensor %u: %u\n", index, get_reading(index));
    }

    return true;
}

void reading_command_help(char prefix)
{
    printf("%csensorX       Displays the ADC reading for sensor X.\n", prefix);
    printf("               Within [0, 8)\n");
}

bool temperature_command(char* message)
{
    if (strlen(message) != 5) {
        return false;
    }
    if (strncmp(message, "temp", 4) != 0) {
        return false;
    }

    int index = (int)(message[4] - '0');
    if (index < 0 || index > 7) {
        printf("\nInvalid sensor index. The values possible are numbers within [0,8)\n");
    } else {
        printf("\nTemperature on sensor %u: %uºC\n", index, get_temperature(index));
    }

    return true;
}

void temperature_command_help(char prefix)
{
    printf("%ctempX         Displays the temperature reading for sensor X, in ºC.\n", prefix);
    printf("               Within [0, 8)\n");
}

void sensor_init()
{
    adc_config_t adc_config;
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 16;
    ESP_ERROR_CHECK(adc_init(&adc_config));

    gpio_config_t gpio_cfg;
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_cfg.pin_bit_mask = CONTROL_PINS_FLAG;
    gpio_cfg.pull_down_en = 0;
    gpio_cfg.pull_up_en = 0;
    gpio_config(&gpio_cfg);

    read_temperatures();

    if (TEST_SENSORS_SPEED) {
        test_speed();
    }
}

void sensors_task()
{
    while (1) {
        read_temperatures();
        vTaskDelay(READ_PERIOD / portTICK_PERIOD_MS);
    }
}