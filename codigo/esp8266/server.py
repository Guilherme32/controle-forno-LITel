from network_handler import NetworkHandler
from sensor_reader import SensorReader
from controller import Controller


def setup(network_handler: NetworkHandler,
          sensor_reader: SensorReader,
          controller: Controller):
    import json
    from nanoweb import HttpError, Nanoweb, send_file
    from sensor_reader import reading_to_temp

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
        await request.write(b"HTTP/1.1 200 Ok\r\n")
        await request.write("Content-Encoding: gzip\r\n")
        await request.write("Content-Type: application/javascript\r\n")
        await request.write("Content-Length: 74041\r\n\r\n")

        await send_file(
            request,
            'd3.v4.min.js.gz',
        )

    async def c3_js(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n")
        await request.write("Content-Encoding: gzip\r\n\r\n")

        await send_file(
            request,
            'c3.min.js.gz',
        )

    async def c3_css(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await send_file(
            request,
            'c3.min.css',
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
            "ssid": network_handler.ap_ssid,
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
    async def controller_info(request):
        await request.write(b"HTTP/1.1 200 Ok\r\n\r\n")

        await request.write(json.dumps({
            "temperatures":
                [sensor_reader.get_temperature(i) for i in range(6)],  # 6 sensores
            "steady_state": bool(controller.steady_pin.value()),
            "power_ratio": controller.power_ratio,
            "control": controller.control,
            "set_point": reading_to_temp(controller.set_point)
        }))

    async def send_power(request):
        if request.method != "POST":
            raise HttpError(request, 501, "Not Implemented")

        size = int(request.headers.get("Content-Length", 0))
        if size == 0:
            await request.write(b"HTTP/1.1 204 No Content\r\n\r\n")
            return

        msg = await request.read(size)
        data = json.loads(msg)
        power = data.get("power", 0)

        if power > controller.period or power < 0:
            await request.write(b"HTTP/1.1 400 Invalid Power")
        else:
            controller.set_ratio((power, controller.period - power))
            await request.write(b"HTTP/1.1 200 Ok")

    async def send_set_point(request):
        if request.method != "POST":
            raise HttpError(request, 501, "Not Implemented")

        size = int(request.headers.get("Content-Length", 0))
        if size == 0:
            await request.write(b"HTTP/1.1 204 No Content\r\n\r\n")
            return

        msg = await request.read(size)
        data = json.loads(msg)
        target = data.get("set_point", 0)

        if target > controller.max_target or target < 0:
            await request.write(b"HTTP/1.1 400 Invalid Set Point")
        else:
            controller.set_target(target)
            await request.write(b"HTTP/1.1 200 Ok")

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
        "/c3.min.js": c3_js,
        "/c3.min.css": c3_css,

        "/api/sta_info": sta_info,
        "/api/ap_info": ap_info,

        "/api/controller_info": controller_info,
        "/api/send_power": send_power,
        "/api/send_set_point": send_set_point
    }

    return app
