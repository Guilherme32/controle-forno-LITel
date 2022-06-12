# TODO implementar o controlador

from machine import Pin
import micropython
import time


class Controller:
    def __init__(self, interrupt_pin, output_pins, read_function, period=120,
                 max_target=140):
        self.output_pins = tuple(Pin(pin, Pin.OUT) for pin in output_pins)
        for pin in self.output_pins:
            pin.value(0)

        # Interrupcao da deteccao de zero
        self.pir = Pin(interrupt_pin, Pin.IN, Pin.PULL_UP)
        self.pir.irq(self.send_power, Pin.IRQ_RISING)

        self.read_sensor = read_function
        self.period = period
                            # ligado, desligado
        self.power_ratio = (0, self.period)
        self.power_counter = 0
        self.cycles = 0

        self.control = False
        self.set_point = 0

        self.max_target = max_target

        self.last_tick = time.ticks_ms()

    @micropython.native
    def send_power(self, _: Pin):
        """ Chamado no interrupt da deteccao de zero """
        new_tick = time.ticks_ms()
        diff = time.ticks_diff(new_tick, self.last_tick)
        if diff < 4:   # debounce
            return
        self.last_tick = new_tick

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

    @micropython.native
    def update_ratio(self):
        if self.control:
            # Decide
            pass

    @micropython.native
    def set_pins(self, value):
        for pin in self.output_pins:
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

    def shutdown(self):
        """ Desliga o controlador e a carga """
        self.control = False
        self.power_counter = 0
        self.power_ratio = (0, self.period)
        self.set_pins(0)

    def set_ratio_command(self, command):
        """ Callback para o comando setp. Seta a relacao de potencia na saida.
        E esperado 3 digitos apos o setp, de 000 a periodo (120 por padrao),
        indicando o tempo ligado.
        """
        if len(command) < 7:  # Ainda não completou o comando
            return False

        on_time = command[4:]
        try:
            on_time = int(on_time)
        except ValueError:
            print("Comando invalido. Deve ser 'setpXXX', onde XXX e o tempo "
                  "ligado, em ciclos.")
            return True

        if 0 <= on_time <= self.period:
            self.set_ratio((on_time, self.period - on_time))
            print("Relacao atualizada")
        else:
            print("Valor invalido. O tempo ligado deve estas entre 0 e periodo"
                  " maximo (120 por padrao)")
        return True

    def set_target_command(self, command):
        """ Callback para o comando sett. Seta a temperatura alvo do sistema.
        E esperado 5 caracteres apos o sett, no formato XXX.X, onde esse valor
        e a temperatura em ºC, com precisao de 1 casa decimal
        """
        if len(command) < 9:  # Ainda não completou o comando
            return False

        target = command[4:]
        try:
            target = float(target)
        except ValueError:
            print("Comando invalido. Deve ser 'settXXX.X', onde XXX.X e a"
                  "temperatura alvo, em ºC")
            return True

        if 0 <= target < self.max_target:
            self.set_target(target)
            print("Set point atualizado")
        else:
            print("Valor invalido. O tempo ligado deve estas entre 0 e a "
                  "temperatura maxima (140ºC por padrao)")
        return True
