from machine import Pin, ADC
from uasyncio import sleep_ms
import micropython


class SensorReader:
    def __init__(self, adc,
                 control_pins,      # type: Tuple[int]
                 read_period: int = 500) -> None:

        self.control_pins = tuple(Pin(pin, Pin.OUT) for pin in control_pins)
        self.read_period = read_period
        self.readings = [0 for _ in range(2 ** len(control_pins))]
        self.adc = ADC(adc)

    def get_temperature(self, index) -> float:
        """Pega a temperatura lida pelo sensor de indice 'index'"""

        voltage = self.readings[index] * 2 / 1024  # *2 pq usa divisor
        return voltage / 0.01       # Sensibilidade no datasheet

    def set_control_pins(self, value):
        """ 
        Seta os pinos de controle do multiplex de forma a acessar o dispositivo 
        numero 'value'
        """
        for c, pin in enumerate(self.control_pins):
            pin.value((value >> c) % 2)

    def read_temperatures(self):
        """Le a temperatura em todos os sensores e salva no objeto"""

        for i in range(len(self.readings)):
            self.set_control_pins(i)
            self.readings[i] = self.adc.read()

    async def run(self):
        """O loop de funcionamento do sistema de sensores"""
        while True:
            self.read_temperatures()
            await sleep_ms(self.read_period)

    def reading_command(self, command) -> bool:
        """ Callback para o comando sensor. Envia de volta a leitura no
        sensor, entre 0 e 1023
        """
        if command == "sensor":     # Ainda não completou o comando
            return False

        sensor_index = command[6:]
        try:
            sensor_index = int(sensor_index)
        except ValueError:
            print("Comando invalido. Deve ser 'sensorX', onde x e o indice do "
                  "sensor")
            return True

        if 0 <= sensor_index < 8:
            print(self.readings[sensor_index])
        else:
            print("Indice invalido. O sistema aceita indices de 0 a 7.")
        return True

    def temperature_command(self, command) -> bool:
        """ Callback para o comando temp. Envia de volta a temperatura lida do
         sensor.
         """
        if command == "temp":  # Ainda não completou o comando
            return False

        sensor_index = command[4:]
        try:
            sensor_index = int(sensor_index)
        except ValueError:
            print("Comando invalido. Deve ser 'tempX', onde x e o indice do "
                  "sensor")
            return True

        if 0 <= sensor_index < 8:
            print(f"{self.get_temperature(sensor_index):.2f}ºC")
        else:
            print("Indice invalido. O sistema aceita indices de 0 a 7.")
        return True
