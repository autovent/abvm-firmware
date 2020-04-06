import pandas as pd
import matplotlib.pyplot as plt


df = pd.read_csv("~/out.csv")
fig, axes = plt.subplots(nrows=2, ncols=1)

df.plot("time", ["pos"], ax=axes[0])
df.plot("time", ["vel", "vel_max"], ax=axes[1])

plt.show()