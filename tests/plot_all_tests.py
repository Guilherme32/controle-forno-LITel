import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import os


folders = list(filter(os.path.isdir, os.listdir()))

for folder in folders:
    files = map(lambda x: os.path.join(folder, x), os.listdir(folder))
    files = filter(os.path.isfile, files)
    files = filter(lambda x: x.endswith(".csv"), files)
    files = list(files)

    if len(files) == 0:
        continue

    fig, ax = plt.subplots(figsize=(15, 5))
    last_time = 0

    for filename in files:
        df = pd.read_csv(filename)
        df["time"] = df["time"] - df["time"][0] + last_time
        last_time = df["time"].max()

        set_point = round(df["set_point"].mean())

        sns.lineplot(data=df, x="time", y="set_point", ax=ax)
        sns.lineplot(data=df, x="time", y="S5", ax=ax)

    ax.set_xlabel("Tempo (s)")
    ax.set_ylabel("Temperatura (ºC)")
    ax.set_title(f"Teste total")

    fig.savefig(f"{folder}.png", dpi=500, facecolor="white")
