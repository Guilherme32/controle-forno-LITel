import os
import sys
import json

from machine import Pin
import machine
import uasyncio
import network

from interrupt_exit import ExitHandler
from serial_comm import SerialHandler
from network_handler import NetworkHandler


async def main():
    print("Entrou no main")

    exit_handler = ExitHandler()
    serial_handler = SerialHandler(("", lambda: None))
    network_handler = NetworkHandler()

    uasyncio.create_task(serial_handler.run())
    uasyncio.create_task(network_handler.run())

    print("Chegou aq")
    ir_pin = Pin(5, Pin.OUT)       # GPIO5 = D1, ir = interrupt
    machine.mem32[0x6000033c] |= 3 << 7     # de interrupt nas duas bordas

    while True:
        if exit_handler.pressed_button:
            exit_handler.exit_program()

        await uasyncio.sleep_ms(100)


if __name__ == "__main__":
    uasyncio.run(main())


# TODO classe do controlador, que usa info do leitor do sensor
# TODO integração com web
