import re
import numpy as np
from scipy.optimize import curve_fit

# Pressure curve data from Kittopia dumps (stock KSP)
bodies = {
    "Eve": {
        "alt":    [0, 15000, 25000, 40000, 50000, 60000, 70000, 80000, 90000],
        "press":  [506.625, 95.6891, 18.07334, 3.5, 0.1217772, 0.02300074, 0.004344278, 0.0008205283, 0.0],
        "P0_guess": 506.625,
        "H_guess": 7000,
    },
    "Kerbin": {
        "alt":    [0, 1241.025, 2439.593, 3597.11, 4714.942, 5794.409, 6836.791,
                   7843.328, 8815.22, 10786.42, 12101.4, 13417.05, 16678.47,
                   21143.1, 26977.92, 33593.82, 42081.87, 49312.13, 56669.95, 62300.84, 70000],
        "press":  [101.325, 84.02916, 69.68138, 57.78001, 47.90862, 39.72148, 32.93169,
                   27.30109, 22.63206, 15.3684, 11.87313, 9.172798, 4.842261,
                   2.050097, 0.6905929, 0.2201734, 0.05768469, 0.01753794, 0.004591824,
                   0.001497072, 0.0],
        "P0_guess": 101.325,
        "H_guess": 5600,
    },
    "Laythe": {
        "alt":    [0, 5250, 10000, 17000, 22000, 31000, 38000, 50000],
        "press":  [60.795, 33.40898, 17.78605, 7.100577, 3.812421, 1.312482, 0.5104055, 0.0],
        "P0_guess": 60.795,
        "H_guess": 4000,
    },
    "Duna": {
        "alt":    [0, 12000, 20000, 35000, 50000],
        "press":  [6.755, 1.276, 0.241, 0.015, 0.0],
        "P0_guess": 6.755,
        "H_guess": 3000,
    },
}

def exp_model(alt, P0, H):
    return P0 * np.exp(-alt / H)

H_values = { }

for name, data in bodies.items():
    alt = np.array(data["alt"])
    press = np.array(data["press"], dtype=float)

    # exclude zero-pressure points from fit
    mask = press > 0
    alt_fit = alt[mask]
    press_fit = press[mask]

    try:
        popt, pcov = curve_fit(exp_model, alt_fit, press_fit,
                               p0=[data["P0_guess"], data["H_guess"]],
                               maxfev=10000)
        P0_fit, H_fit = popt
        perr = np.sqrt(np.diag(pcov))
        pred = exp_model(alt_fit, *popt)
        residuals = press_fit - pred
        rmse = np.sqrt(np.mean(residuals**2))
        r2 = 1 - np.sum(residuals**2) / np.sum((press_fit - np.mean(press_fit))**2)
    except Exception as e:
        print(f"{name}: fit failed — {e}")
        continue

    print(f"{'='*55}")
    print(f"  {name}")
    print(f"{'='*55}")
    print(f"  P0 = {P0_fit:.4f} ± {perr[0]:.4f} kPa  (actual: {data['P0_guess']})")
    print(f"  H  = {H_fit:.1f} ± {perr[1]:.1f} m")
    print(f"  RMSE = {rmse:.4f} kPa")
    print(f"  R²   = {r2:.6f}")
    print()

    H_values[name] = H_fit

    # Show per-point comparison
    print(f"  {'alt (m)':>8s}  {'actual':>8s}  {'fitted':>8s}  {'error%':>7s}")
    for a, p_act in zip(alt_fit, press_fit):
        p_pred = exp_model(a, *popt)
        err = (p_pred - p_act) / p_act * 100
        print(f"  {a:8.0f}  {p_act:8.4f}  {p_pred:8.4f}  {err:+6.2f}%")
    print()
