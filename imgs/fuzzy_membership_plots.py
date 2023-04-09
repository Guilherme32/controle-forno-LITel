import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np


# -----------------------------------------------------------------------------


def fuzzify_triangular(value: int, center: int,
                       half_width: int, edge: int) -> int:
    if edge == -1 and value < center:
        return 256
    if edge == 1 and value > center:
        return 256
    if value > center + half_width:
        return 0
    if value < center - half_width:
        return 0

    centered_value = int(abs(value - center))
    return int((256 * (half_width - centered_value)) // half_width)


def reading_to_temp(reading: int) -> float:
    voltage = reading * 2 / 1024  # *2 pq usa divisor
    return voltage / 0.01


# -----------------------------------------------------------------------------


readings = np.array(range(0, round(1024*0.75)))
temps = np.array([reading_to_temp(reading) for reading in readings])

plt.plot(readings, temps)
plt.show()


# -----------------------------------------------------------------------------


readings = np.array(range(-30, 30))
temps = np.array([reading_to_temp(reading) for reading in readings])

delta_temp_vars = {"Z": (0, 15, 0),
                   "N": (-15, 15, -1),
                   "P": (15, 15, 1)}

plt.title("$\mu$ para as variáveis de $\Delta T$")
plt.xlabel("$\Delta T$ (ºC)")
plt.ylabel("$\mu$")

for var_name, var_info in delta_temp_vars.items():
    plt.plot(temps,
             np.array([fuzzify_triangular(read, *var_info) for read in readings])/256,
             label=var_name)


# https://stackoverflow.com/questions/22263807/how-is-order-of-items-in-matplotlib-legend-determined
handles, labels = plt.gca().get_legend_handles_labels()
order = [1, 0, 2]
plt.legend([handles[idx] for idx in order], [labels[idx] for idx in order])

plt.savefig("deltaT.svg", facecolor="white")


# -----------------------------------------------------------------------------


plt.figure()
readings = np.array(range(-175, 75))
temps = np.array([reading_to_temp(reading) for reading in readings])

delta_temp_vars = {"NM": (-100, 50, 0),
                   "NS": (-50, 50, 0),
                   "Z": (0, 50, 0),
                   "P": (50, 50, 1),
                   "NL": (-150, 50, -1)}

plt.title("$\mu$ para as variáveis de erro")
plt.xlabel("$e_T$ (ºC)")
plt.ylabel("$\mu$")

for var_name, var_info in delta_temp_vars.items():
    plt.plot(temps,
             np.array([fuzzify_triangular(read, *var_info) for read in readings])/256,
             label=var_name)


# https://stackoverflow.com/questions/22263807/how-is-order-of-items-in-matplotlib-legend-determined
handles, labels = plt.gca().get_legend_handles_labels()
order = [4, 0, 1, 2, 3]
plt.legend([handles[idx] for idx in order], [labels[idx] for idx in order])

plt.savefig("erro.svg", facecolor="white")


# -----------------------------------------------------------------------------


plt.figure()
delta_temp_vars = {"ST": 0,
                   "L": 10,
                   "M": 30,
                   "Z": 0,
                   "H": 40}

plt.title("$\mu$ para a saída principal")
plt.xlabel("$P$ (%)")
plt.ylabel("$\mu$")

for i, item in enumerate(delta_temp_vars.items()):
    var_name, value = item
    plt.stem([value/1.2, ],
             [1, ],
             label=var_name,
             linefmt=f"C{i}-",
             markerfmt=f"C{i}o")

plt.xlim((-5, 55))
plt.ylim((0, 1.1))

# https://stackoverflow.com/questions/22263807/how-is-order-of-items-in-matplotlib-legend-determined
handles, labels = plt.gca().get_legend_handles_labels()
order = [4, 0, 1, 2, 3]
plt.legend([handles[idx] for idx in order], [labels[idx] for idx in order])

plt.savefig("saida.svg", facecolor="white")


# -----------------------------------------------------------------------------


plt.figure()
delta_temp_vars = {"N": -4,
                   "Z": 0,
                   "P": 4,}

plt.title("$\mu$ para a saída integradora")
plt.xlabel("$\Delta P$ (%)")
plt.ylabel("$\mu$")

for i, item in enumerate(delta_temp_vars.items()):
    var_name, value = item
    plt.stem([value/1.2, ],
             [1, ],
             label=var_name,
             linefmt=f"C{i}-",
             markerfmt=f"C{i}o")

plt.xlim((-6, 6))
plt.ylim((0, 1.1))

# https://stackoverflow.com/questions/22263807/how-is-order-of-items-in-matplotlib-legend-determined
handles, labels = plt.gca().get_legend_handles_labels()
order = [0, 1, 2]
plt.legend([handles[idx] for idx in order], [labels[idx] for idx in order])

plt.savefig("saida_i.svg", facecolor="white")
