from machine import Pin
import machine
import uasyncio

import gc

from interrupt_exit import ExitHandler
from serial_comm import SerialHandler
from network_handler import NetworkHandler
from sensor_reader import SensorReader


async def main():
    print("\n\nPrograma do controle do forno do LITel")
    commands = []

    exit_handler = ExitHandler()

    network_handler = NetworkHandler()
    commands.append(("netinfo", network_handler.info_command))
    uasyncio.create_task(network_handler.run())

    sensor_reader = SensorReader(0, (14, 13, 12))
    commands.append(("temp", sensor_reader.temperature_command))
    commands.append(("sensor", sensor_reader.reading_command))
    uasyncio.create_task(sensor_reader.run())

    serial_handler = SerialHandler(commands)
    uasyncio.create_task(serial_handler.run())

    ir_pin = Pin(5, Pin.OUT)       # GPIO5 = D1, ir = interrupt
    machine.mem32[0x6000033c] |= 3 << 7     # de interrupt nas duas bordas

    while True:
        if exit_handler.pressed_button:
            exit_handler.exit_program()

        await uasyncio.sleep_ms(100)


if __name__ == "__main__":
    uasyncio.run(main())


# TODO integração com web
