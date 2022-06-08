import select
import sys
from uasyncio import sleep_ms

# from typing import Tuple, Callable    # descomentar para escrever


class SerialHandler:
    def __init__(self,
                 commands,         # type: Tuple[str, Callable]
                 command_prefix="!",
                 command_max_size=8):
        self.poll = select.poll()
        self.poll.register(sys.stdin, select.POLLIN)

        self.commands = list(commands)
        self.command_prefix = command_prefix
        self.command_max_size = command_max_size

        self.msg = ""

    def check_msg(self) -> None:
        """ Checa a mensagem, vendo se tem um comando com o seu nome. É chamada
        quando um caracter novo chega.

        Nota: Como só é considerado a partir do momento em que o prefixo foi
        recebido, o tamanho mínimo nessa função é 2.
        """

        _msg = self.msg[1:]
        if len(_msg) > self.command_max_size:
            self.msg = ""
        else:
            for command in self.commands:
                if command[0] in _msg:
                    if command[1](_msg):
                        self.msg = ""

    def add_command(self, name, function):
        """ Adiciona um comando. O nome e uma string com a mensagem para
        chamar o comando, a função deve receber um str e retornar um bool.
        Deve ser True se funcionou e False se o comando for inválido.
        """
        self.commands.append((name, function))

    async def run(self):
        """ Roda a comunicação. Espera caracteres na entrada do sistema. Nas
        configurações padrões do micropython, o canal de comunicação padrão é
        UART em 115200 b/s, com 8 bits de dados, nenhum de paridade e
        1 stop bit. """
        while True:
            await sleep_ms(50)
            while events := self.poll.poll(0):
                for event in events:
                    char = event[0].read(1)

                    if char == self.command_prefix:
                        self.msg = self.command_prefix

                    elif self.msg:      # se já tem algo na msg
                        self.msg += char
                        self.check_msg()
