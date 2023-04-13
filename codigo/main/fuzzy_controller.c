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

static int fuzzy_accumulator[3];       // N, Z, P
static int accumulator_count;
static int accumulated_power;


// Static functions declaration -------------------------------------------------------------------

static int IRAM_ATTR fuzzify_triangular(int value, int center, int half_width, int edge);
static int IRAM_ATTR min(int value1, int value2);
static int IRAM_ATTR max(int value1, int value2);
static int IRAM_ATTR max3(int value1, int value2, int value3);
static int IRAM_ATTR abs(int value);
static void IRAM_ATTR fuzzify_delta_temp(int sensor_reading);
static void IRAM_ATTR fuzzify_error(int sensor_reading);
static void IRAM_ATTR calculate_power();
static void IRAM_ATTR defuzzify_power(int ambient_reading);
static void IRAM_ATTR calculate_accumulator();
static void IRAM_ATTR defuzzify_accumulator();


// Static functions definition --------------------------------------------------------------------

static int IRAM_ATTR fuzzify_triangular(int value, int center, int half_width, int edge)
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

static int IRAM_ATTR min(int value1, int value2)
{
    return value1 < value2 ? value1 : value2;
}

static int IRAM_ATTR max(int value1, int value2)
{
    return value1 > value2 ? value1 : value2;
}

static int IRAM_ATTR max3(int value1, int value2, int value3)
{
    int _max = value1 > value2 ? value1 : value2;
    _max = value3 > _max ? value3 : _max;

    return _max;
}

static int IRAM_ATTR abs(int value)
{
    return value > 0 ? value : -value;
}

static void IRAM_ATTR fuzzify_delta_temp(int sensor_reading)
{
    int delta_temp = sensor_reading - last_read;
    last_read = sensor_reading;

    fuzzy_delta_temp[0] = fuzzify_triangular(delta_temp, -15, 15, -1);
    fuzzy_delta_temp[1] = fuzzify_triangular(delta_temp, 0,   15, 0 ),
    fuzzy_delta_temp[2] = fuzzify_triangular(delta_temp, 15,  15, 1 );
}

static void IRAM_ATTR fuzzify_error(int sensor_reading)
{
    int error = sensor_reading - set_point;

    fuzzy_error[0] = fuzzify_triangular(error, -150, 50, -1);
    fuzzy_error[1] = fuzzify_triangular(error, -100, 50,  0);
    fuzzy_error[2] = fuzzify_triangular(error, -50,  50,  0);
    fuzzy_error[3] = fuzzify_triangular(error,  0,   50,  0);
    fuzzy_error[4] = fuzzify_triangular(error,  50,  50,  1);
}

static void IRAM_ATTR calculate_power()
{
    fuzzy_power[0] = max(fuzzy_error[E_P],
                         fuzzy_delta_temp[DT_P]);

    fuzzy_power[1] = max(fuzzy_error[E_Z],
                         min(fuzzy_error[E_NS], fuzzy_delta_temp[DT_P]));

    fuzzy_power[2] = max(fuzzy_error[E_NM],
                         min(fuzzy_error[E_NS], fuzzy_delta_temp[DT_Z]));

    fuzzy_power[3] = max(fuzzy_error[E_NL],
                         min(fuzzy_error[E_NM], fuzzy_delta_temp[DT_Z]));

    fuzzy_power[4] = min(fuzzy_error[E_NL], fuzzy_delta_temp[DT_Z]);
}

static void IRAM_ATTR defuzzify_power(int ambient_reading)
{
    int correction_value = (set_point - ambient_reading) / 21;

    int w_power[5] = {
        0,
        correction_value,
        10 + correction_value,
        30 + correction_value,
        40 + correction_value,
    };

    int _power = 0;
    int membership_sum = 0;

    for (int i=0; i<5; i++) {
        _power += fuzzy_power[i] * w_power[i];
        membership_sum += fuzzy_power[i];
    }

    if (membership_sum != 0) {
        _power /= membership_sum;
    }
    power = _power;

    if (power > max_power) {
        power = max_power;
    }
    if (power < 0) {
        power = 0;
    }
}

static void IRAM_ATTR calculate_accumulator()
{
    fuzzy_accumulator[0] = min(fuzzy_delta_temp[DT_Z], fuzzy_error[E_P]);

    fuzzy_accumulator[1] = max3(fuzzy_error[E_Z],
                               fuzzy_delta_temp[DT_N],
                               fuzzy_delta_temp[DT_P]);

    fuzzy_accumulator[2] = min(fuzzy_delta_temp[DT_Z], fuzzy_error[E_NS]);
}

static void IRAM_ATTR defuzzify_accumulator()
{
    // The values are scaled by a factor of a 100 to virtually increase resolution

    int w_accumulator[3] = {-400, 0, 400};

    int accumulator = 0;
    int membership_sum = 0;

    for (int i=0; i<3; i++) {
        accumulator += fuzzy_accumulator[i] * w_accumulator[i];
        membership_sum += fuzzy_accumulator[i];
    }

    if (membership_sum != 0) {
        accumulator /= membership_sum;
    }

    accumulated_power += accumulator;

    if (accumulated_power > 400) {
        accumulated_power = 400;
    }
    if (accumulated_power < -400) {
        accumulated_power = -400;
    }
}


// Public functions definition --------------------------------------------------------------------

int IRAM_ATTR run_fuzzy_step(int sensor_reading, int ambient_reading, bool accumulate)
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

    fuzzy_delta_temp[0] = 0;
    fuzzy_delta_temp[1] = 0;
    fuzzy_delta_temp[2] = 0;

    fuzzy_error[0] = 0;
    fuzzy_error[1] = 0;
    fuzzy_error[2] = 0;
    fuzzy_error[3] = 0;
    fuzzy_error[4] = 0;

    fuzzy_power[0] = 0;
    fuzzy_power[1] = 0;
    fuzzy_power[2] = 0;
    fuzzy_power[3] = 0;
    fuzzy_power[4] = 0;

    power = 0;

    fuzzy_accumulator[0] = 0;
    fuzzy_accumulator[1] = 0;
    fuzzy_accumulator[2] = 0;

    accumulator_count = 0;
    accumulated_power = 0;
}
