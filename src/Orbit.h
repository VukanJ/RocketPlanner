#ifndef ORBIT_H
#define ORBIT_H

#include "Eigen/Core"
#include <utility>
struct Body;

using vec3f = Eigen::Vector3f;

struct Orbit {
    static Orbit circular(const Body* ref, float R);
    static Orbit elliptic(const Body* ref, float ap, float pe);
    static Orbit fromLambert(const Body* ref, vec3f r1, vec3f r2, vec3f Fprime, float a);

    const Body* parent = nullptr;
    float LAN = 0;  // Longitude of ascending node Ω
    float LDN = 180;  // Longitude of descending node Ω + 180°
    float argumentOfPeriapsis = 0;  // ω
    float meanAnomaly = 0;  // M
    double AP = 0;
    double PE = 0;
    double a_semi = 0; // a
    float eccentricity = 0; // e
    float inclination = 0; // i
    float epoch = 0; // Seconds elapsed at t = 0

    Eigen::Vector3f normal() const;
    float v_apoapsis(float unit) const;
    float v_periapsis(float unit) const;
    float period(float unit=1) const;
    float trueAnomalyAt(const vec3f& r) const;

};

std::pair<vec3f, vec3f> get_local_position(const Orbit& o);
std::pair<vec3f, vec3f> get_local_future_position(const Orbit& o, std::uint64_t seconds);

float default_transfer_time_estimate(const Body* from, const Body* to);
float default_transfer_window_estimate(const Body* from, const Body* to);

#endif // ORBIT_H
