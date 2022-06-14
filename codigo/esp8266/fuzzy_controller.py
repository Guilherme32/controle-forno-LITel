import micropython

NL = micropython.const(0)
NS = micropython.const(1)
Z  = micropython.const(2)
PS = micropython.const(3)
PL = micropython.const(4)


@micropython.viper
def fuzzify_triangular(value: int, center: int,
                       half_width: int, edge: int) -> int:
    if edge == -1 and value < center:
        return 256
    if edge == 1 and value > center:
        return 256
    if value > center + half_width:
        return 0
    if value < center - half_width:
        return 0

    centered_value = int(abs(value - center))
    return int((256 * (half_width - centered_value)) // half_width)


@micropython.viper
def _min(value1: int, value2: int) -> int:
    return value1 if value1 < value2 else value2


@micropython.viper
def _max(value1: int, value2: int) -> int:
    return value1 if value1 > value2 else value2


class FuzzyController:
    def __init__(self, temp, max_power):
        self.set_point = 0
        self.last_temp = temp

                               # N, Z, P
        self.fuzzy_delta_temp = (0, 0, 0)
                          # NL, NS, Z, PS, PL
        self.fuzzy_error = (0,  0,  0, 0,  0)
                              # NL, NS, Z, PS, PL
        self.fuzzy_power_add = (0, 0, 0, 0, 0)

        self.power = 0
        self.max_power = max_power

    def set_target(self, value):
        self.set_point = value

    def fuzzify_delta_temp(self, temp: int) -> None:
        delta_temp = int(temp - self.last_temp)
        self.last_temp = temp

        self.fuzzy_delta_temp = (
            fuzzify_triangular(delta_temp, -6, 6, -1),
            fuzzify_triangular(delta_temp, 0, 6, 0),
            fuzzify_triangular(delta_temp, 6, 6, 1)
        )

    def fuzzify_error(self, temp: int) -> None:
        error = temp - int(self.set_point)
        self.fuzzy_error = (
            fuzzify_triangular(error, -103, 52, -1),
            fuzzify_triangular(error, -52,  52,  0),
            fuzzify_triangular(error,  0,   52,  0),
            fuzzify_triangular(error,  52,  52,  0),
            fuzzify_triangular(error,  103, 52,  1)
        )

    def calculate_power(self):
        self.fuzzy_power_add = (
            _max(self.fuzzy_error[PL],
                 _min(self.fuzzy_error[PS], self.fuzzy_delta_temp[2])),

            _max(_min(self.fuzzy_error[PS], self.fuzzy_delta_temp[1]),
                 _min(self.fuzzy_error[Z], self.fuzzy_delta_temp[2])),

            max(_min(self.fuzzy_error[Z], self.fuzzy_delta_temp[1]),
                _min(self.fuzzy_error[NS], self.fuzzy_delta_temp[2]),
                _min(self.fuzzy_error[PS], self.fuzzy_delta_temp[0])),

            _max(_min(self.fuzzy_error[NS], self.fuzzy_delta_temp[1]),
                 _min(self.fuzzy_error[Z], self.fuzzy_delta_temp[0])),

            _max(self.fuzzy_error[NL],
                 _min(self.fuzzy_error[NS], self.fuzzy_delta_temp[0])),
        )

    def deffuzify_power(self):
        w_power_add = (-30, -15, 0, 15, 30)     # Support member value

        power_add = 0
        membership_sum = 0
        for i in range(5):
            power_add += self.fuzzy_power_add[i] * w_power_add[i]
            membership_sum += self.fuzzy_power_add[i]

        power_add //= membership_sum

        self.power += power_add

        if self.power > self.max_power:
            self.power = self.max_power
        if self.power < 0:
            self.power = 0

    def run_step(self, temp):
        self.fuzzify_error(temp)
        self.fuzzify_delta_temp(temp)
        self.calculate_power()
        self.deffuzify_power()

        return self.power
