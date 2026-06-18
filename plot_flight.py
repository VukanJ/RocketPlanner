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

def plot_stage():
    for t in stage_t:
        plt.axvline(x=t, color='r', linestyle='-', linewidth=0.5)

plt.subplot(3, 1, 1)
plt.plot(df.t, df.alt_km, label='alt_km')
plt.legend(loc="upper left")
plot_stage()

plt.subplot(3, 1, 2)
plt.plot(df.t, df.apoapsis_km, label='apoapsis_km')
plt.legend(loc="upper left")
plot_stage()

plt.subplot(3, 1, 3)
plt.plot(df.t, df.dir_angle_deg, label='dir_angle_deg')
plt.legend(loc="upper left")
plot_stage()
plt.savefig('alt_apoapsis.pdf')

plt.tight_layout()
plt.close()

plt.plot(df.posx_km, df.posy_km)
plt.savefig('trajectory.pdf')
plt.close()

plt.plot(df.vx_m_s, df.vy_m_s)
plt.savefig('vel.pdf')
plt.close()

plt.plot(df.t, df.mass_t)
plot_stage()
plt.savefig('mass.pdf')
plt.close()
