from requests import get, post
from time import sleep, time
import pandas as pd
import os


step_values = list(range(40, 141, 20))
step_time = 10
sample_time = 1
ip_addr = "192.168.0.101"
back_forth = True

init_time = time()
total_time = step_time * len(step_values)

print("Teste de funcionamento em escada do forno")

i = 0
while os.path.isdir(f"test_{str(i).zfill(2)}"):
    i += 1

folder_path = f"test_{str(i).zfill(2)}"
os.mkdir(folder_path)

for i, value in enumerate(step_values):
    this_time = time() - init_time
    df = pd.DataFrame()
    post(f"http://{ip_addr}/api/send_set_point", json={"set_point": value})

    while this_time < step_time * (i + 1):
        sleep(sample_time)
        this_time = time() - init_time
        print(f"\rTestando {value}ºC ({i+1}/{len(step_values)}), "
              f"tempo restante: {(total_time - this_time)/60:.1f}min", end="")

        resp = get(f"http://{ip_addr}/api/controller_info")
        resp = resp.json()
        set_point = resp["set_point"]
        resp = {f"S{x+1}": y for x, y in enumerate(resp["temperatures"])}
        resp["set_point"] = set_point
        resp["time"] = this_time
        resp = {key: [value, ] for key, value in resp.items()}

        df = pd.concat((df, pd.DataFrame.from_dict(resp)), ignore_index=True)

    df.to_csv(f"{folder_path}/test_{i}_{value}.csv")

print("\rTeste finalizado com sucesso")
