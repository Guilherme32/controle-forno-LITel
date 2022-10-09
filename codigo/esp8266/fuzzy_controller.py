import micropython

NL = micropython.const(0)
NM = micropython.const(1)
NS  = micropython.const(2)
Z = micropython.const(3)
P = micropython.const(4)


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
        self.last_temp = temp               # 0~1024, a leitura do sensor

                               # N, Z, P
        self.fuzzy_delta_temp = (0, 0, 0)
                          # NL, NM, NS, Z, P
        self.fuzzy_error = (0,  0,  0, 0,  0)
                         # Z, ST, L, M, H
        self.fuzzy_power = (0, 0, 0, 0, 0)

        self.power = 0
        self.max_power = max_power

        self.accumulator_count = 0
                                # N, Z, P
        self.fuzzy_accumulator = (0, 0, 0)
        self.accumulated_power = 0

    def set_target(self, value):
        self.set_point = value

    def fuzzify_delta_temp(self, temp: int) -> None:
        delta_temp = int(temp - self.last_temp)
        self.last_temp = temp

        self.fuzzy_delta_temp = (
            fuzzify_triangular(delta_temp, -15, 15, -1),
            fuzzify_triangular(delta_temp, 0, 15, 0),
            fuzzify_triangular(delta_temp, 15, 15, 1)
        )

    def fuzzify_error(self, temp: int) -> None:
        error = temp - int(self.set_point)
        self.fuzzy_error = (
            fuzzify_triangular(error, -150, 50, -1),
            fuzzify_triangular(error, -100, 50,  0),
            fuzzify_triangular(error, -50, 50,  0),
            fuzzify_triangular(error,  0,  50,  0),
            fuzzify_triangular(error,  50, 50,  1)
        )

    def calculate_power(self):
        self.fuzzy_power = (
            max(self.fuzzy_error[P],
                self.fuzzy_delta_temp[2]),

            max(self.fuzzy_error[Z],
                min(self.fuzzy_error[NS], self.fuzzy_delta_temp[2])),

            max(self.fuzzy_error[NM],
                min(self.fuzzy_error[NS], self.fuzzy_delta_temp[1])),

            max(self.fuzzy_error[NL],
                min(self.fuzzy_error[NM], self.fuzzy_delta_temp[1])),

            min(self.fuzzy_error[NL], self.fuzzy_delta_temp[1])
        )

    def deffuzify_power(self, room_temp):
        correction_value = (self.set_point - room_temp) // 21
        w_power = (0,
                   correction_value,
                   10 + correction_value,
                   30 + correction_value,
                   40 + correction_value)     # Support member value
        power = 0
        membership_sum = 0
        for i in range(5):
            power += self.fuzzy_power[i] * w_power[i]
            membership_sum += self.fuzzy_power[i]

        power //= membership_sum

        self.power = power

        if self.power > self.max_power:
            self.power = self.max_power
        if self.power < 0:
            self.power = 0

    def calculate_accumulator(self):
        self.fuzzy_accumulator = (
            min(self.fuzzy_delta_temp[1], self.fuzzy_error[P]),

            max(self.fuzzy_error[Z],
                self.fuzzy_delta_temp[0],
                self.fuzzy_delta_temp[2]),

            min(self.fuzzy_delta_temp[1], self.fuzzy_error[NS])
        )

    def deffuzify_accumulator(self):
        # Esses valores estão escalados por 100 para aumentar a resolução de
        # forma virtual
        w_accumulator = (-400, 0, 400)

        accumulator = 0
        membership_sum = 0
        for i in range(3):
            accumulator += self.fuzzy_accumulator[i] * w_accumulator[i]
            membership_sum += self.fuzzy_accumulator[i]

        accumulator //= membership_sum
        self.accumulated_power += accumulator

        if self.accumulated_power > 400:
            self.accumulated_power = 400

        if self.accumulated_power < -400:
            self.accumulated_power = -400

    def run_step(self, temp: int, room_temp: int, accumulate: bool):
        self.fuzzify_error(temp)
        self.fuzzify_delta_temp(temp)
        self.calculate_power()
        self.deffuzify_power(room_temp)

        if accumulate:
            if self.accumulator_count == 0:
                self.calculate_accumulator()
                self.accumulate()
            self.accumulator_count += 1
            self.accumulator_count %= 4
        else:
            self.accumulator_count = 0

        return self.power + self.accumulated_power//100
