from machine import Pin


class Controller:
    def __init__(self, pins, read_function, period=120):
        self.pins = tuple(Pin(pin, Pin.OUT) for pin in pins)
        for pin in self.pins:
            pin.value(0)

        self.read_sensor = read_function
        self.period = period
                            # ligado, desligado
        self.power_ratio = (1, self.period - 1)    # deve ter pelo menos 1 em cada lado
        self.power_counter = 0
        self.cycles = 0

        self.control = True
        self.set_point = 0

    def send_power(self):
        if self.cycles == self.period:
            self.cycles = 0
            self.update_ratio()

        if self.power_counter > 0:
            self.power_counter -= self.power_ratio[1]
            self.set_pins(1)
        else:
            self.power_counter += self.power_ratio[0]
            self.set_pins(0)

        self.cycles += 1

    def update_ratio(self):
        if self.control:
            # Decide
            pass

    def set_pins(self, value):
        for pin in self.pins:
            pin.value(value)

    def set_ratio(self, ratio: tuple):
        """ Coloca a potencia de saida, e desativa o modo de controle """
        self.power_ratio = ratio
        self.check_counter_limits()
        self.control = False

    def set_target(self, target: float):
        """ Coloca o set point, como a leitura esperada no adc do sensor, e
        ativa o modo de controle
        """
        self.set_point = int(target * 5e-3 * 1024)
        self.control = True
        self.update_ratio()
        self.check_counter_limits()

    def check_counter_limits(self):
        """ Confere se o power counter esta nos limites da relacao de
        potencia. Deve ser chamado quando a relacao for alterada quando o
        contador e diferente de 0 para ter uma mudanca mais suave.
        """
        if self.power_counter > self.power_ratio[0]:
            self.power_counter = self.power_ratio[0]

        if self.power_counter < - self.power_ratio[1]:
            self.power_counter = - self.power_ratio[1]
