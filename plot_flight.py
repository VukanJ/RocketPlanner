import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

df = pd.read_csv('build/flight_log.csv', sep=',')

stage = 0
stage_t = []
for i in range(len(df.t)):
    if df.stage[i] > stage:
        stage = df.stage[i]
        stage_t.append(df.t[i]-1)

plt.plot(df.t, df.apoapsis_km)
for t in stage_t:
    plt.axvline(x=t, color='r', linestyle='--')
plt.show()
