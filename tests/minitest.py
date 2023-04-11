from requests import get, post
from time import sleep, time
import pandas as pd
import os


def read_to_temp(reading: int) -> float:
    voltage = (reading * 2) / 1023
    temp = voltage / 0.01
    return temp


step_values = list(range(60, 141, 20))
step_values = [120, ]
step_time = 25 * 60
sample_time = 1
ip_addr = "192.168.0.101"

init_time = time()
total_time = step_time * len(step_values)

print("Teste de funcionamento em escada do forno")

i = 0
while os.path.isdir(f"test_{str(i).zfill(2)}"):
    i += 1

folder_path = f"test_{str(i).zfill(2)}"
os.mkdir(folder_path)

fails = 0

for i, value in enumerate(step_values):
    this_time = time() - init_time
    df = pd.DataFrame()
    post(f"http://{ip_addr}/api/send_set_point", json={"set_point": value})

    while this_time < step_time * (i + 1):
        sleep(sample_time)
        this_time = time() - init_time
        print(f"\rTestando {value}ºC ({i+1}/{len(step_values)}), "
              f"tempo restante: {(total_time - this_time)/60:.1f}min"
              f" - failed {fails}         ", end="")

        try:
            resp = get(f"http://{ip_addr}/api/controller_info", timeout=2)
            resp = resp.json()
            set_point = resp["set_point"]
            power = resp["power_ratio"]
            resp = {f"S{x+1}": read_to_temp(y) for x, y in enumerate(resp["readings"])}
            resp["set_point"] = set_point
            resp["power"] = power
            resp["time"] = this_time
            resp = {key: [value, ] for key, value in resp.items()}

            df = pd.concat((df, pd.DataFrame.from_dict(resp)), ignore_index=True)
        except:
            fails += 1

    df.to_csv(f"{folder_path}/test_{i}_{str(value).zfill(3)}.csv")

print("\rTeste finalizado com sucesso")
