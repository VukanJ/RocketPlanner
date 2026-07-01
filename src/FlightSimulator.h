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

    void clear() {
        t.clear();
        altitude_km.clear();
        velocity_ms.clear();
        vx_ms.clear();
        vy_ms.clear();
        posx_km.clear();
        posy_km.clear();
        thrust_kN.clear();
        mass_t.clear();
        pressure_atm.clear();
        dir_angle_deg.clear();
        apoapsis_km.clear();
        stage.clear();
        area_m2.clear();
        drag_N.clear();
    }
};

struct LaunchSuccess {
    bool apoapsis_safe_height = false;
    float circularization_dv = INFINITY;
    float availableDeltaV = 0;
    float timeToApoapsis = INFINITY;
    float burnTimeRequired = INFINITY;
};

template <typename F=float>
struct vec2 { F x = 0; F y = 0; };

using vec2f = vec2<float>;

float getApoapsis(const vec2<float>& pos, const vec2<float>& vel, float r_km, float GM, float body_R);
float getPeriapsis(const vec2<float>& pos, const vec2<float>& vel, float r_km, float GM, float body_R);

enum ApoPer {Apoapsis, Periapsis};

template <ApoPer AP, typename FLT=float>
FLT getOrbitExtent(const vec2<FLT>& pos, const vec2<FLT>& vel, FLT r_km, FLT GM) {
    FLT vx = vel.x / 1000.0;
    FLT vy = vel.y / 1000.0;
    FLT v2 = vx*vx + vy*vy;
    FLT eps = 0.5f * v2 - GM / r_km;
    if (eps >= 0) { return INFINITY; }
    FLT a   = -GM / (2.0f * eps);
    FLT h   = pos.x * vy - pos.y * vx;
    FLT e   = std::sqrt(1.0f + 2.0f*eps*h*h / (GM * GM));
    if constexpr (AP == Apoapsis) {
        return a * (1.0f + e);
    }
    else if constexpr (AP == Periapsis){
        return a * (1.0f - e);
    }
    else {
        static_assert(true, "Invalid option");
    }
}


class FlightSimulator {
public:
    FlightSimulator();
    enum class SimOpt { FAST, RECORD };

    template <SimOpt SIM>
    void simulate_launch(const Body& body, const RocketConfig& rocket,
                         float gtClimbAlt = 0.0f, 
                         float gtTurnSpread = 1.0f, 
                         float gtFinalPitch = 85.0f);

    float dt = 0.1;
    const double Cd = 0.2; // Drag Coefficient

    FlightData<float> flight_data;
    LaunchSuccess launchSuccess;
};

#endif // FLIGHT_SIMULATOR_H
