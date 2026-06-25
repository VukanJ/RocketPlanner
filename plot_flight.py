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

def get_stage_points():
    stage_indices = np.where(df.stage.diff() > 0)[0]
    return stage_indices-1

points = get_stage_points()

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
for t in points:
    plt.plot(df.posx_km.iloc[t], df.posy_km.iloc[t], marker='x', markersize=5, color='r')
plt.savefig('trajectory.pdf')
plt.close()

plt.plot(df.vx_m_s, df.vy_m_s)
for t in points:
    plt.plot(df.vx_m_s.iloc[t], df.vy_m_s.iloc[t], marker='x', markersize=5, color='r')
plt.savefig('vel.pdf')
plt.close()

plt.plot(df.t, df.mass_t)
plot_stage()
plt.savefig('mass.pdf')
plt.close()

plt.subplot(1, 2, 1)
plt.plot(df.posx_km, df.vx_m_s*df.mass_t)
plt.xlabel('posx_km')
plt.ylabel('momentum_x')
for t in points:
    plt.plot(df.posx_km.iloc[t], df.vx_m_s.iloc[t]*df.mass_t.iloc[t], marker='x', markersize=5, color='r')
plt.subplot(1, 2, 2)
plt.plot(df.posy_km, df.vy_m_s*df.mass_t)
plt.xlabel('posy_km')
plt.ylabel('momentum_y')
for t in points:
    plt.plot(df.posy_km.iloc[t], df.vy_m_s.iloc[t]*df.mass_t.iloc[t], marker='x', markersize=5, color='r')
plt.savefig('phase.pdf')
plt.close()

plt.plot(df.t, df.drag_N)
plot_stage()
plt.savefig('drag.pdf')
plt.close()

plt.plot(df.t, df.A_m2)
plot_stage()
plt.savefig('A.pdf')
plt.close()
