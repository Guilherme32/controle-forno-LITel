import uasyncio

import gc

gc.collect()
from controller import Controller
gc.collect()
from interrupt_exit import ExitHandler
gc.collect()
from serial_comm import SerialHandler
gc.collect()
from network_handler import NetworkHandler
gc.collect()
from sensor_reader import SensorReader
gc.collect()

import server

gc.collect()


async def main():
    print("\n\nPrograma do controle do forno do LITel")
    print("Carregando...")
    commands = []

    exit_handler = ExitHandler(0)

    network_handler = NetworkHandler(2)
    commands.append(("netinfo", network_handler.info_command))
    uasyncio.create_task(network_handler.run())

    sensor_reader = SensorReader(0, (12, 14, 16))
    commands.append(("temp", sensor_reader.temperature_command))
    commands.append(("sensor", sensor_reader.reading_command))
    uasyncio.create_task(sensor_reader.run())

    controller = Controller(13, (4, 5), 15, lambda: sensor_reader.readings[4])
    commands.append(("sett", controller.set_target_command))
    commands.append(("setp", controller.set_ratio_command))

    serial_handler = SerialHandler(commands)
    uasyncio.create_task(serial_handler.run())

    gc.collect()

    app = server.setup(network_handler, sensor_reader, controller)
    uasyncio.create_task(app.run())

    gc.collect()
    print(f"{gc.mem_free()} bytes livres apos inicio do webserver")

    print("Carregado.")
    while True:
        if exit_handler.pressed_button:
            controller.shutdown()
            exit_handler.exit_program()

        await uasyncio.sleep_ms(1000)
        gc.collect()


if __name__ == "__main__":
    uasyncio.run(main())
