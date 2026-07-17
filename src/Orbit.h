#ifndef ORBIT_H
#define ORBIT_H

#include "Eigen/Core"
#include <utility>
struct Body;

using vec3f = Eigen::Vector3f;

struct Orbit {
    static constexpr Orbit circular(const Body* ref, float R) {
        Orbit o;
        o.parent = ref;
        o.AP = R;
        o.PE = R;
        o.a_semi = R;
        return o;
    }

    static constexpr Orbit elliptic(const Body* ref, float ap, float pe) {
        if (pe > ap) {
            throw std::runtime_error("PE <= AP is required");
        }
        Orbit o;
        o.parent = ref;
        o.AP = ap;
        o.PE = pe;
        o.a_semi = 0.5 * (ap + pe);

        float b = std::sqrt(ap * pe);

        o.eccentricity = std::sqrt(1 - b*b / (o.a_semi * o.a_semi));
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
    float period() const;

};

std::pair<vec3f, vec3f> get_local_position(const Orbit& o);
std::pair<vec3f, vec3f> get_local_future_position(const Orbit& o, std::uint64_t seconds);

#endif // ORBIT_H
