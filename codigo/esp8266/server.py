from network_handler import NetworkHandler
from sensor_reader import SensorReader
from controller import Controller


def setup(network_handler: NetworkHandler,
          sensor_reader: SensorReader,
          controller: Controller):
    import tinyweb

    app = tinyweb.webserver()

    # region static
    @app.route("/")
    @app.route("/index.html")
    async def index(request, response):
        """ Pagina principal """
        await response.send_file("index.html")

    @app.route("/connection.html")
    async def connection(request, response):
        """ Pagina de conexao """
        await response.send_file("connection.html")

    @app.route("/control.html")
    async def control(request, response):
        """ Pagina de controle """
        await response.send_file("control.html")

    @app.route("/style.css")
    async def style(request, response):
        await response.send_file("style.css", content_type="text/css")

    @app.route("/fire_icon.svg")
    async def fire_icon(request, response):
        await response.send_file("fire_icon.svg", content_type="image/svg")

    @app.route("/d3.v4.min.js")
    async def fire_icon(request, response):
        await response.send_file("d3.v4.min.js",
                                 content_type="application/javascript")

    # endregion

    class APInfo:
        def get(self, data):
            pass

    class STAInfo:
        def get(self, data):
            pass

        def post(self, data):
            pass

    app.add_resource(APInfo, "/api/ap_info")
    app.add_resource(STAInfo, "/api/sta_info")

    return app
