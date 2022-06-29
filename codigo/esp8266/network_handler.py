import json
import os
import uasyncio
import network
from machine import Pin


class NetworkHandler:
    def __init__(self, status_pin,
                 ap_ssid="forno-litel",
                 ap_password="787Cu7kg",
                 port=80,
                 tries=10) -> None:
        """
        Cuida das questoes de rede, da conexao wifi tanto em modo acess point
        (o dispositivo se conecta a rede do esp) quanto em modo station (o
        esp e o dispositivo se conectam  em um intermediario - o roteador)
        """
        self.ap_ssid = ap_ssid
        self.ap_password = ap_password

        self.ap = network.WLAN(network.AP_IF)
        self.ap.config(essid=ap_ssid,
                       password=ap_password)
        self.ap.active(True)
        
        self.sta_ssid = ""
        self.sta_password = ""

        self.sta = network.WLAN(network.STA_IF)
        self.sta.active(False)

        self.port = port

        self.max_tries = tries
        self.tries = 0

        self.load_config()

        self.status_pin = Pin(status_pin, Pin.OUT)
        self.status_pin.value(0)

    def load_config(self) -> None:
        """Carrega o config.json, que guarda o ssid e senha pra conectar"""

        if "config.json" in os.listdir():
            with open("config.json", "r") as file:
                config: dict = json.load(file)
                self.sta_ssid = config.get("sta_ssid", "")
                self.sta_password = config.get("sta_password", "")

    async def connect(self) -> bool:
        """
        Tenta conectar a rede pela configuracao dada.
        Tenta uma quantidade de vezes passada na criacao do handler,
        esperando 30s entre cada tentativa
        """
        self.sta.active(True)
        self.sta.connect(self.sta_ssid, self.sta_password)
        while self.sta.status() == network.STAT_CONNECTING:
            self.status_pin.value(not self.status_pin.value())
            await uasyncio.sleep_ms(100)

        if self.sta.isconnected():
            self.tries = 0
            self.status_pin.value(1)
            return True

        self.status_pin.value(0)

        self.tries += 1
        if self.tries >= self.max_tries:
            self.sta.active(False)
            return False

        await uasyncio.sleep(30)
        return await self.connect()

    async def run(self):
        """
        O loop geral do handler. So confere se ainda esta conectado para
        mandar tentar de novo
        """
        while True:
            if (not self.sta.isconnected()) and self.sta_ssid:
                self.status_pin.value(0)
                await self.connect()
            await uasyncio.sleep(1)

    def ap_info(self) -> str:
        """Retorna uma string com as informacoes do modo ap formatadas"""
        ifconfig = self.ap.ifconfig()
        return f"AP: SSID: {self.ap_ssid}    Senha: {self.ap_password}    " \
               + f"IP: {ifconfig[0]}"

    def sta_info(self) -> str:
        """Retorna uma string com as informacoes do modo sta formatadas"""
        if self.sta.isconnected():
            ifconfig = self.sta.ifconfig()
            return f"STA: SSID: {self.sta_ssid}    IP: {ifconfig[0]}"

        return "STA desconectado"

    def all_info(self) -> str:
        """Retorna uma string com todas as informacoes de conexao"""
        return f"{self.ap_info()}\n" \
               + f"{self.sta_info()}\n" \
               + f"Escutando na porta {self.port}"

    def info_command(self, _):
        """Envia as informacoes de conexao pelo serial"""
        print(self.all_info())
        return True

    def update_config(self, ssid: str, password: str) -> bool:
        """
        Atualiza a configuracao de rede, e desativa o modo sta. Assim o
        loop principal vai tentar reconectar. Retorna True se ssid e password
        foram validos, false se nao
        """

        if ssid and password:
            config = {}
            if "config.json" in os.listdir():
                with open("config.json", "r") as file:
                    config = json.load(file)

            config["sta_ssid"] = ssid
            config["sta_password"] = password
            with open("config.json", "w") as file:
                json.dump(config, file)

            self.sta.active(False)
            self.load_config()

            return True

        return False
