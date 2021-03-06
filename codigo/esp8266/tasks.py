import sys

from invoke import task, context


port = "COM4"

@task()
def send(c, filename, _compile=False, main=False, boot=False):
    # Procurar a possibilidade de um Enum aqui ^^^^
    c: context.Context
    filename: str

    if boot and main:
        print("As opcoes --main e --boot sao mutuamente exclusivas. O arquivo"
              "nao pode ser enviado como ambos.")
        sys.exit(0)

    # Compila para bytecode (mpy), na arquitetura do esp
    if _compile:
        command = f"mpy-cross -march=xtensa {filename}"
        print(f"> {command}")
        c.run(command)

        filename = filename[:-3]
        filename += ".mpy"      # Usando o compilado como novo arquivo

    command = f"ampy -p {port} -b 115200 put {filename}"
    command += " main.py" if main else ""
    command += " boot.py" if boot else ""

    print(f"> {command}")
    c.run(command)

    if _compile:
        command = f"rm {filename}"
        print(f"> {command}")
        c.run(command)


@task()
def ls(c):
    command = f"ampy -b 115200 -p {port} ls"
    print(f"> {command}")
    c.run(command)


@task()
def rm(c, filename):
    command = f"ampy -b 115200 -p {port} rm {filename}"
    print(f"> {command}")
    c.run(command)


@task()
def mkdir(c, dirname):
    command = f"ampy -b 115200 -p {port} mkdir {dirname}"
    print(f"> {command}")
    c.run(command)


@task()
def send_all(c):
    try:
        rm(c, "main.py")
    except:
        pass

    send(c, "controller.py", _compile=True)
    send(c, "fuzzy_controller.py", _compile=True)
    send(c, "interrupt_exit.py", _compile=True)
    send(c, "network_handler.py", _compile=True)
    send(c, "sensor_reader.py", _compile=True)
    send(c, "serial_comm.py", _compile=True)
    send(c, "server.py", _compile=True)
    send(c, "nanoweb.py", _compile=True)

    send(c, "web/index.html")
    send(c, "web/control.html")
    send(c, "web/connection.html")
    send(c, "web/fire_icon.svg")
    send(c, "web/style.css")
    send(c, "web/d3.v4.min.js.gz")
    send(c, "web/c3.min.js.gz")
    send(c, "web/c3.min.css")
    send(c, "web/connection.js")
    send(c, "web/control.js")

    send(c, "main.py", _compile=False)
