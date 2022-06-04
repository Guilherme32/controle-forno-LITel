from machine import Pin


class ExitHandler:
    def __init__(self, gpio=0):     # gpio0 é conectado a flash
        self.pressed_button = False

        pir = Pin(gpio, Pin.IN)
        pir.irq(trigger=Pin.IRQ_RISING, handler=self.button_press)

    def button_press(self, pin: Pin):
        self.pressed_button = True

    def exit_program(self):
        raise SystemExit("Exit button pressed")
