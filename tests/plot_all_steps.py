import os
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt


filenames = []

for x in os.walk("."):
    path = x[0]
    for file in [f for f in x[2] if f.endswith(".csv")]:
        filenames.append(os.path.join(path, file))

for filename in filenames:
    df = pd.read_csv(filename)
    df["time"] = df["time"] - df["time"][0]
    set_point = round(df["set_point"].mean())
    
    fig, ax = plt.subplots()

    sns.lineplot(data=df, x="time", y="set_point", ax=ax)
    sns.lineplot(data=df, x="time", y="S5", ax=ax)
    
    xlim = ax.get_xlim()
    x = xlim[0] + (xlim[1] - xlim[0]) * 0.6
    ylim = ax.get_ylim()
    y = ylim[0] + (ylim[1] - ylim[0]) * 0.25

    tail = df.where(df["time"] > (df["time"].max() - 300))

    ax.text(x, y, f"Média final: {tail['S5'].mean():.1f}ºC")
    
    ax.set_xlabel("Tempo (s)")
    ax.set_ylabel("Temperatura (ºC)")
    ax.set_title(f"Teste para set point = {set_point}ºC")

    fig.savefig(f"{filename[:-4]}.png", dpi=500, facecolor="white")
