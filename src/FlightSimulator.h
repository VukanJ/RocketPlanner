#ifndef FLIGHT_SIMULATOR_H
#define FLIGHT_SIMULATOR_H

#include <vector>
#include <cmath>
#include "kspConstants.h"
#include "RocketConfig.h"

template <typename FLT>
struct FlightData {
    std::vector<FLT> t;
    std::vector<FLT> altitude_km;
    std::vector<FLT> velocity_ms;
    std::vector<FLT> vx_ms;
    std::vector<FLT> vy_ms;
    std::vector<FLT> posx_km;
    std::vector<FLT> posy_km;
    std::vector<FLT> thrust_kN;
    std::vector<FLT> mass_t;
    std::vector<FLT> pressure_atm;
    std::vector<FLT> dir_angle_deg;
    std::vector<FLT> apoapsis_km;
    std::vector<int> stage;
    std::vector<FLT> area_m2;
    std::vector<FLT> drag_N;

    void reserve(std::size_t n) {
        t.reserve(n);
        altitude_km.reserve(n);
        velocity_ms.reserve(n);
        vx_ms.reserve(n);
        vy_ms.reserve(n);
        posx_km.reserve(n);
        posy_km.reserve(n);
        thrust_kN.reserve(n);
        mass_t.reserve(n);
        pressure_atm.reserve(n);
        dir_angle_deg.reserve(n);
        apoapsis_km.reserve(n);
        stage.reserve(n);
        area_m2.reserve(n);
        drag_N.reserve(n);
    }
};

struct LaunchSuccess {
    bool apoapsis_safe_height = false;
    float circularization_dv = INFINITY;
    float availableDeltaV = 0;
    float timeToApoapsis = INFINITY;
    float burnTimeRequired = INFINITY;
};

FlightData<float> simulate_flight(Body body, const RocketConfig& rocket, LaunchSuccess&);

#endif // FLIGHT_SIMULATOR_H
