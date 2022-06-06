import json
import os
import uasyncio
import network


class NetworkHandler:
    def __init__(self, ap_ssid="forno-litel",
                 ap_password="787Cu7kg",
                 tries=10) -> None:
        
        self.ap = network.WLAN(network.AP_IF)
        self.ap.config(essid=ap_ssid,
                       password=ap_password)
        self.ap.active(True)
        
        self.sta_ssid = ""
        self.sta_password = ""

        self.sta = network.WLAN(network.STA_IF)
        self.sta.active(False)

        self.max_tries = tries
        self.tries = 0

        self.load_config()

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
            await uasyncio.sleep_ms(100)

        if self.sta.isconnected():
            self.tries = 0
            return True

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
                await self.connect()
            await uasyncio.sleep(10)

    def ap_info(self):
        return self.ap.ifconfig()

    def sta_info(self):
        return self.sta.config() \
            if self.sta.isconnected() \
            else None
