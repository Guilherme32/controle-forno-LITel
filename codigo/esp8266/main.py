import sys

from machine import Pin, Timer
import time
import machine
import uasyncio
import select

from interrupt_exit import ExitHandler
from serial_comm import SerialHandler


def timer_callback(msg):
    global button_latch
    global led

    button_latch = False
    led.value(not led.value())


def button_pressed(pin: Pin):
    global led
    led.value(pin.value())


async def main():
    print("Entrou no main")

    exit_handler = ExitHandler()
    serial_handler = SerialHandler(("", lambda: None))
    await uasyncio.create_task(serial_handler.run())

    print("Chegou aq")
    ir_pin = Pin(5, Pin.OUT)       # GPIO5 = D1, ir = interrupt

    ir_pin.irq(handler=button_pressed)      # O micropython não tem a opção
    machine.mem32[0x6000033c] |= 3 << 7     # de interrupt nas duas bordas

    while True:
        if exit_handler.pressed_button:
            exit_handler.exit_program()

        await uasyncio.sleep_ms(100)


button_latch = False
# Nao e recomendado usar o timer 0 pq ele esta envolvido com o wifi
timer = Timer(-1)
led = Pin(4, Pin.OUT)
u_secs = 0

if __name__ == "__main__":
    uasyncio.run(main())


# TODO classe para leitor de sensor, que fica lendo a uma taxa constante
# TODO classe para comunicação serial
#  que aceita uma lista de comandos e uma lista de callbacks
# TODO classe do controlador, que usa info do leitor do sensor