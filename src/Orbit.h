#ifndef ORBIT_H
#define ORBIT_H

#include "Eigen/Core"
struct Body;

struct Orbit {
    static constexpr Orbit circular(const Body* ref, float R) {
        Orbit o;
        o.parent = ref;
        o.AP = R;
        o.PE = R;
        o.a_semi = R;
        return o;
    }
    static constexpr Orbit hyperbolic(const Body* ref, float R) {
        Orbit o;
        o.parent = ref;
        o.AP = R;
        o.PE = R;
        o.a_semi = R;
        o.eccentricity = 1.5;
        return o;
    }

    const Body* parent = nullptr;
    float LAN = 0;  // Longitude of ascending node
    float LDN = 180;  // Longitude of descending node
    float argumentOfPeriapsis = 0;
    float meanAnomaly = 0;
    double AP = 0;
    double PE = 0;
    double a_semi = 0;
    float eccentricity = 0;
    float inclination = 0;
    float epoch = 0; // Seconds elapsed at t = 0

    Eigen::Vector3f normal() const;
    float v_apoapsis(float unit) const;
    float v_periapsis(float unit) const;
};

#endif // ORBIT_H
