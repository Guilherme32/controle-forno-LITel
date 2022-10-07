from machine import Pin
import micropython
import time

from fuzzy_controller import FuzzyController

# untested - change in steady state detection

class Controller:
    def __init__(self, interrupt_pin: int,
                 output_pins,      # type: Tuple[int]
                 stability_pin: int,
                 read_function,     # type: Callable
                 read_room_function,    # type: Callable
                 period=120,
                 max_target=140):
        """
        Cuida das questoes do controlador:
            - Enviar a potencia de saida determinada
            - Chamar o algoritmo de decisao periodicamente

        O algoritmo de controle propriamente dito (de decisao) usado e um com
        fuzzy logic, em fuzzy_controller.py

        Para modular a potencia, uma estrategia de zero-cross e empregada.
        O controlador ativa a conducao do scr em uma quantidade X de
        semiciclos, enquanto bloqueia em Y semiciclos. O padrao e alinhar
        esses semiciclos de forma sequencial, mas aqui e usada uma forma que
        permite uma distribuicao mais homogenea dentro do periodo
        """
        self.output_pins = tuple(Pin(pin, Pin.OUT) for pin in output_pins)
        for pin in self.output_pins:
            pin.value(0)

        # Interrupcao da deteccao de zero
        self.pir = Pin(interrupt_pin, Pin.IN, Pin.PULL_UP)
        self.pir.irq(self.send_power, Pin.IRQ_RISING)

        self.read_sensor = read_function
        self.read_room = read_room_function
        self.period = period
                            # ligado, desligado
        self.power_ratio = (0, self.period)
        self.power_counter = 0
        self.cycles = 0

        self.control = False
        self.set_point = 0

        self.max_target = max_target

        self.last_tick = time.ticks_ms()

        self.fuzzy_controller = FuzzyController(read_function(), self.period)
        self.last_read = read_function()

        self.steady_count = 0
        self.steady_pin = Pin(stability_pin, Pin.OUT)
        self.steady_pin.value(0)

    @micropython.native
    def send_power(self, _: Pin):
        """ Chamado no interrupt da deteccao de zero """
        new_tick = time.ticks_ms()
        diff = time.ticks_diff(new_tick, self.last_tick)
        if diff < 4:   # debounce
            return
        self.last_tick = new_tick

        if self.cycles % self.period == 0:
            self.check_steady_state()

        if self.cycles >= (30 * self.period):
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
        """Atualiza a relacao de potencia a ser enviada a carga. O acumulador
        só é invocado quando o sistema já alcançou o regime permanente mas ainda
        não alcançou o set point"""
        if self.control:
            power = self.fuzzy_controller.run_step(
                self.read_sensor(),
                self.read_room(),
                self.steady_pin.value() \
                    and (abs(self.read_sensor() - self.set_point) > 4))
            self.power_ratio = (power, self.period - power)

    def check_steady_state(self):
        """
        Confere se alcancou o regime permanente. Se chegou, indica no led
        correspondente
        """
        if -4 <= self.read_sensor() - self.last_read <= 4:
            self.steady_count += 1
        else:
            self.last_read = self.read_sensor()
            self.steady_count = 0

        if self.steady_count >= 60:
            self.steady_pin.value(1)
        else:
            self.steady_pin.value(0)

    def set_pins(self, value: int):
        """Manda o valor para todas as saidas do controlador"""
        for pin in self.output_pins:
            pin.value(value)

    def set_ratio(self, ratio: tuple):
        """Seta a potencia de saida, e desativa o modo de controle"""
        self.power_ratio = ratio
        self.check_counter_limits()
        self.control = False

    def set_target(self, target: float):
        """
        Transforma o parametro recebido de ºC em um inteiro representando a
        leitura correspondente a mesma temperatura. Entao seta o set point, e
        ativa o modo de controle
        """
        self.set_point = int(target * 5e-3 * 1024)
        self.fuzzy_controller.set_target(self.set_point)
        self.fuzzy_controller.accumulated_power = 0
        self.control = True
        self.update_ratio()
        self.check_counter_limits()

    def check_counter_limits(self):
        """
        Confere se o power counter esta nos limites da relacao de
        potencia. Deve ser chamado quando a relacao for alterada quando o
        contador e diferente de 0 para ter uma mudanca mais suave
        """
        if self.power_counter > self.power_ratio[0]:
            self.power_counter = self.power_ratio[0]

        if self.power_counter < - self.power_ratio[1]:
            self.power_counter = - self.power_ratio[1]

    def shutdown(self):
        """Desliga o controlador e a carga"""
        self.control = False
        self.power_counter = 0
        self.power_ratio = (0, self.period)
        self.set_pins(0)

    def set_ratio_command(self, command: str) -> bool:
        """
        Callback para o comando setp. Seta a relacao de potencia na saida.
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

    def set_target_command(self, command: str) -> bool:
        """
        Callback para o comando sett. Seta a temperatura alvo do sistema.
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
