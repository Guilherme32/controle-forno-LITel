from machine import Pin, ADC
from uasyncio import sleep_ms


class SensorReader:
    def __init__(self, adc,
                 *control_pins,      # type: Tuple[int]
                 read_period: int = 500) -> None:

        self.control_pins = tuple(Pin(pin, Pin.OUT) for pin in control_pins)
        self.read_period = read_period
        self.readings = [0 for 0 in range(2 ** len(control_pins))]
        self.adc = ADC(adc)

    def get_temperature(self, index):
        voltage = self.readings[index] * 2 * 1000/1024  # *2 pq usa divisor
        return 2 + voltage / 0.01       # Sensibilidade no datasheet

    def set_control_pins(self, value):
        for c, pin in enumerate(self.control_pins):
            pin.value((value >> c) % 2)

    def read_temperatures(self):
        for i in range(len(self.readings)):
            self.set_control_pins(i)
            self.readings[i] = self.adc.read()

    async def run(self):
        while True:
            self.read_temperatures()
            await sleep_ms(self.read_period)

# TODO testar
# Untested, tem que esperar o multiplex chegar
