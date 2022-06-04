import select
import sys
from uasyncio import sleep_ms

# from typing import Tuple, Callable    # descomentar para escrever


class SerialHandler:
    def __init__(self,
                 *commands,         # : Tuple[str, Callable],
                 command_prefix="!",
                 command_max_size=9):
        self.poll = select.poll()
        self.poll.register(sys.stdin, select.POLLIN)

        self.commands = commands
        self.command_prefix = command_prefix
        self.command_max_size = command_max_size

        self.msg = b""

    def check_msg(self):
        print(self.msg)
        if len(self.msg) >= self.command_max_size:
            self.msg = ""

    async def run(self):
        while events := self.poll.poll(0):
            await sleep_ms(50)
            for event in events:
                char = event[0].read(1)

                if char == self.command_prefix:
                    self.msg = self.command_prefix

                elif char[0] == self.command_prefix:
                    self.msg += char
                    self.check_msg()
