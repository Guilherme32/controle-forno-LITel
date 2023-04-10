#include "fuzzy_controller.h"


// Definitions ------------------------------------------------------------------------------------

#define DT_N 0
#define DT_Z 1
#define DT_P 2

#define E_NL 0
#define E_NM 1
#define E_NS 2
#define E_Z  3
#define E_P  4

#define P_Z  0
#define P_ST 1
#define P_L  2
#define P_M  3
#define P_H  4

#define AP_N 0
#define AP_Z 1
#define AP_P 2


// Global variables declaration -------------------------------------------------------------------

static int set_point;
static int last_read;
static int max_power;

static int fuzzy_delta_temp[3];        // N, Z, P
static int fuzzy_error[5];             // NL, NM, NS, Z, P
static int fuzzy_power[5];             // Z, ST, L, M, H
static int power;

static int fuzzy_acummulator[3];       // N, Z, P
static int accumulator_count;
static int accumulated_power;


// Static functions declaration -------------------------------------------------------------------

static int fuzzify_triangular(int value, int center, int half_width, int edge);
static int min(int value1, int value2);
static int max(int value1, int value2);
static void fuzzify_delta_temp(int sensor_reading);
static void fuzzify_error(int sensor_reading);
static void calculate_power();
static void defuzzify_power(int ambient_reading);
static void calculate_accumulator();
static void defuzzify_accumulator();


// Static functions definition --------------------------------------------------------------------

static int fuzzify_triangular(int value, int center, int half_width, int edge)
{
    if (edge == -1 && value < center) {
        return 256;
    }
    if (edge == 1 && value > center) {
        return 256;
    }
    if (value > center + half_width) {
        return 0;
    }
    if (value < center - half_width) {
        return 0;
    }

    int centered_value = abs(value - center);
    return (256 * (half_width - centered_value)) / half_width;
}

static int min(int value1, int value2)
{
    return value1 < value2 ? value1 : value2;
}

static int max(int value1, int value2)
{
    return value1 > value2 ? value1 : value2;
}

static void fuzzify_delta_temp(int sensor_reading)
{
    int delta_temp = sensor_reading - last_read;
    last_read = sensor_reading;

    fuzzy_delta_temp = {
        fuzzify_triangular(delta_temp, -15, 15, -1),
        fuzzify_triangular(delta_temp, 0,   15, 0 ),
        fuzzify_triangular(delta_temp, 15,  15, 1 )
    };
}

static void fuzzify_error(int sensor_reading)
{
    int error = temp - set_point;

    fuzzy_error = {
        fuzzify_triangular(error, -150, 50, -1),
        fuzzify_triangular(error, -100, 50,  0),
        fuzzify_triangular(error, -50,  50,  0),
        fuzzify_triangular(error,  0,   50,  0),
        fuzzify_triangular(error,  50,  50,  1)
    };
}

static void calculate_power()
{
    fuzzy_power = {
        max(fuzzy_error[E_P],
            fuzzy_delta_temp[DT_P]),

        max(fuzzy_error[E_Z],
            min(fuzzy_error[E_NS], fuzzy_delta_temp[DT_P])),

        max(fuzzy_error[E_NM],
            min(fuzzy_error[E_NS], fuzzy_delta_temp[DT_Z])),

        max(fuzzy_error[E_NL],
            min(fuzzy_error[E_NM], fuzzy_delta_temp[DT_Z])),

        min(fuzzy_error[E_NL], fuzzy_delta_temp[DT_Z])
    };
}

static void defuzzify_power(int ambient_reading);
static void calculate_accumulator();
static void defuzzify_accumulator();


// Public functions definition --------------------------------------------------------------------

int run_fuzzy_step(int sensor_reading, int ambient_reading, bool accumulate)
{
    fuzzify_error(sensor_reading);
    fuzzify_delta_temp(sensor_reading);
    calculate_power();
    defuzzify_power(ambient_reading);

    if (accumulate) {
        if (accumulator_count == 0) {
            calculate_accumulator();
            defuzzify_accumulator();
        }
        accumulator_count ++;
        accumulator_count %= 4;
    } else {
        accumulator_count = 0;
    }

    return power + accumulated_power/100;
}

void set_fuzzy_target(int _target)
{
    set_point = _target;
}

void fuzzy_init(int _last_read, int _max_power)
{
    set_point = 0;
    last_read = _last_read;
    max_power = _max_power;

    fuzzy_delta_temp = {0, 0, 0};
    fuzzy_error = {0, 0, 0, 0, 0};
    fuzzy_power = {0, 0, 0, 0, 0};
    power = 0;

    fuzzy_acummulator = {0, 0, 0};
    accumulator_count = 0;
    accumulated_power = 0;
}
