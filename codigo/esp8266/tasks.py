import sys

from invoke import task, context


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

    command = f"ampy -p COM3 -b 115200 put {filename}"
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
    command = "ampy -b 115200 -p COM3 ls"
    print(f"> {command}")
    c.run(command)


@task()
def rm(c, filename):
    command = f"ampy -b 115200 -p COM3 rm {filename}"
    print(f"> {command}")
    c.run(command)
