#!/usr/bin/env python3
"""Fetch Kittopia config dumps and print C++ Body initializers."""

import re
import subprocess
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from ksp_atmo_fit import H_values

BODIES = [
    ("Kerbol", "None"),
    ("Moho",   "KspSystem::Kerbol"),
    ("Eve",    "KspSystem::Kerbol"),
    ("Gilly",  "KspSystem::Eve"),
    ("Kerbin", "KspSystem::Kerbol"),
    ("Mun",    "KspSystem::Kerbin"),
    ("Minmus", "KspSystem::Kerbin"),
    ("Duna",   "KspSystem::Kerbol"),
    ("Ike",    "KspSystem::Duna"),
    ("Dres",   "KspSystem::Kerbol"),
    ("Jool",   "KspSystem::Kerbol"),
    ("Laythe", "KspSystem::Jool"),
    ("Vall",   "KspSystem::Jool"),
    ("Tylo",   "KspSystem::Jool"),
    ("Bop",    "KspSystem::Jool"),
    ("Pol",    "KspSystem::Jool"),
    ("Eeloo",  "KspSystem::Kerbol"),
]

BASE_URL = "https://raw.githubusercontent.com/Kopernicus/kittopia-dumps/master/Configs"
GEE_TO_MS2 = 9.80665
KPA_PER_ATM = 101.325


def fetch_config(name):
    url = f"{BASE_URL}/{name}.cfg"
    result = subprocess.run(
        ["curl", "-s", "-S", "--max-time", "15", url],
        capture_output=True, text=True, timeout=20)
    result.check_returncode()
    return result.stdout


def extract(text, key):
    m = re.search(rf"(?:^|\n)\s*{key}\s*=\s*([0-9.eE+\-]+)", text)
    return float(m.group(1)) if m else None


def fmt32(v):
    if v is None:
        return "0.0f"
    s = f"{v:.15g}"
    if "." not in s and "e" not in s.lower():
        s += ".0"
    return s


def fmt64(v):
    if v is None:
        return "0.0"
    s = f"{v:.15g}"
    if "." not in s and "e" not in s.lower():
        s += ".0"
    return s


def print_body(name, parent_str):
    fetch_name = "Sun" if name == "Kerbol" else name

    text = fetch_config(fetch_name)

    grav_param = extract(text, "gravParameter")
    radius_m   = extract(text, "radius")
    gee_asl    = extract(text, "geeASL")
    rot_period = extract(text, "rotationPeriod")
    soi_m      = extract(text, "sphereOfInfluence")

    has_atm = re.search(r"enabled\s*=\s*True", text) is not None
    kpa               = extract(text, "staticPressureASL")
    atm_height_m      = extract(text, "atmosphereDepth")
    sea_level_density = extract(text, "staticDensityASL")
    atm_falloff       = H_values[name] * 1e-3 if name in H_values else None

    inclination  = extract(text, "inclination")
    eccentricity = extract(text, "eccentricity")
    sma_m        = extract(text, "semiMajorAxis")
    lan          = extract(text, "longitudeOfAscendingNode")
    arg_pe       = extract(text, "argumentOfPeriapsis")
    mean_anomaly = extract(text, "meanAnomalyAtEpoch")
    epoch        = extract(text, "epoch")

    has_orbit = sma_m is not None

    GM_km3s2            = grav_param / 1e9 if grav_param else 0.0
    surface_gravity_ms2 = (gee_asl or 0.0) * GEE_TO_MS2
    radius_km           = (radius_m or 0.0) / 1000.0
    soi_km              = (soi_m or 0.0) / 1000.0 if soi_m else 0.0
    if has_orbit:
        sma_km = sma_m / 1000.0
        e = eccentricity or 0.0
        ap_km = sma_km * (1.0 + e)
        pe_km = sma_km * (1.0 - e)
    else:
        sma_km = 0.0
        e = 0.0
        ap_km = 0.0
        pe_km = 0.0

    # Convert atmosphere values to correct units
    if has_atm and kpa and kpa != 0.0:
        sea_level_atm = kpa / KPA_PER_ATM
        atm_height_km = atm_height_m / 1000.0
        if atm_falloff is None:
            atm_falloff = 0.0
    else:
        sea_level_atm = 0.0
        atm_height_km = 0.0
        atm_falloff = 0.0
        sea_level_density = 0.0

    soi_str = f"{fmt32(soi_km)}" if soi_m else "INFINITY"
    orb_peer = f"&{parent_str}" if parent_str != "None" else "nullptr"
    if parent_str == "None":
        soi_str = "INFINITY"

    print(f"    constexpr inline Body {name}  {{")
    print(f'        .name                    = "{name}",')
    print(f"        .radius_km               = {fmt32(radius_km)},")
    print(f"        .R_SOI_km                = {soi_str},")
    print(f"        .surfaceGravity          = {fmt64(surface_gravity_ms2)},")
    print(f"        .seaLevel_atm            = {fmt32(sea_level_atm)},")
    print(f"        .atmHeight_km            = {fmt32(atm_height_km)},")
    print(f"        .atm_falloff_km          = {fmt32(atm_falloff)},")
    print(f"        .sea_level_density_kgpm3 = {fmt32(sea_level_density)},")
    print(f"        .rotPeriod_s             = {fmt32(rot_period)},")
    print(f"        .GM_km3s2                = {fmt64(GM_km3s2)},")
    if has_orbit:
        print(f"        .orbit={{.parent={orb_peer},")
        print(f"                .LAN                 = {fmt32(lan)},")
        print(f"                .LDN                 = {fmt32((lan or 0) + 180)},")
        print(f"                .argumentOfPeriapsis = {fmt32(arg_pe)},")
        print(f"                .meanAnomaly         = {fmt32(mean_anomaly)},")
        print(f"                .AP                  = {fmt64(ap_km)},")
        print(f"                .PE                  = {fmt64(pe_km)},")
        print(f"                .a_semi              = {fmt64(sma_km)},")
        print(f"                .eccentricity        = {fmt32(eccentricity)},")
        print(f"                .inclination         = {fmt32(inclination)},")
        print(f"                .epoch               = {fmt32(epoch)}")
        print(f"        }} }};")
    else:
        print(f"        .orbit                   = {{}} }};")
    print()


def main():
    print("// Auto-generated by scripts/fetch_kittopia.py")
    print("// Source: Kopernicus/kittopia-dumps")
    print()
    print("#pragma once")
    print()
    for name, parent in BODIES:
        print_body(name, parent)


if __name__ == "__main__":
    main()
