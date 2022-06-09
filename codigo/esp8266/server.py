def setup():
    import tinyweb

    app = tinyweb.webserver()

    # region static
    @app.route("/")
    @app.route("/index.html")
    async def index(request, response):
        """ Pagina principal """
        await response.send_file("web/index.html")

    @app.route("/connection.html")
    async def connection(request, response):
        """ Pagina de conexao """
        await response.send_file("web/connection.html")

    @app.route("/control.html")
    async def control(request, response):
        """ Pagina de controle """
        await response.send_file("web/control.html")

    @app.route("/style.css")
    async def style(request, response):
        await response.send_file("web/style.css", content_type="text/css")

    @app.route("/fire_icon.svg")
    async def fire_icon(request, response):
        await response.send_file("web/fire_icon.svg", content_type="image/svg")

    @app.route("/d3.v4.min.js")
    async def fire_icon(request, response):
        await response.send_file("web//d3.v4.min.js",
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

    def run():
        app.add_resource(APInfo, "/api/ap_info")
        app.add_resource(STAInfo, "/api/sta_info")
        app.run(host='0.0.0.0', port=80)

    return run
