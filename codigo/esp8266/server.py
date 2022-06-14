from network_handler import NetworkHandler
from sensor_reader import SensorReader
from controller import Controller
import json


def setup(network_handler: NetworkHandler,
          sensor_reader: SensorReader,
          controller: Controller):
    import json
    import os
    import time
    import sys
    import uasyncio as asyncio
    from nanoweb import HttpError, Nanoweb, send_file
    from ubinascii import a2b_base64 as base64_decode

    # region static
    async def index(request):
        """ Pagina principal """
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'index.html',
        )

    async def connection(request):
        """ Pagina de conexao """
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'connection.html',
        )

    async def control(request):
        """ Pagina de controle """
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'control.html',
        )

    async def style(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'style.css',
        )

    async def fire_icon(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'fire_icon.svg',
        )

    async def d3(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'd3.v4.min.js',
        )

    async def connection_js(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'connection.js',
        )

    # endregion

    # region network api
    async def ap_info(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await request.write(json.dumps({
            "ssid": network_handler.sta_ssid,
            "password": network_handler.ap_password,
            "ip": network_handler.ap.ifconfig()[0]
        }))

    async def sta_info(request):
        if request.method == "GET":
            await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

            await request.write(json.dumps({
                "ssid": network_handler.sta_ssid,
                "password": network_handler.sta_password,
                "connected": network_handler.sta.isconnected(),
                "ip": network_handler.sta.ifconfig()[0]
            }))
        elif request.method == "POST":
            size = int(request.headers.get("Content-Length", 0))
            if size == 0:
                await request.write("HTTP/1.1 204 No Content\r\n\r\n")
                return

            msg = await request.read(size)
            data = json.loads(msg)
            ssid = data.get("ssid")
            password = data.get("password")

            await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")
            await request.close()
            network_handler.update_config(ssid, password)

    # endregion

    # region sensor and controller api
    # endregion

    app = Nanoweb()
    app.routes = {
        "/": index,
        "/index.html": index,
        "/connection.html": connection,
        "/connection.js": connection_js,
        "/control.html": control,
        "/style.css": style,
        "/fire_icon.svg": fire_icon,
        "/d3.v4.min.js": d3,
        "/api/sta_info": sta_info,
        "/api/ap_info": ap_info
    }

    return app
