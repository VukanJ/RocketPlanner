#include "Orbit.h"
#include "kspConstants.h"
#include "eigen3/Eigen/Geometry"

#include <cmath>

inline constexpr float deg2rad(float f) {
    return f * M_PI / 180.0f;
}

float Orbit::v_apoapsis(float unit) const {
    return unit * std::sqrt(parent->GM_km3s2 * (2.0f / AP - 1.0f / a_semi));
}

float Orbit::v_periapsis(float unit) const {
    return unit * std::sqrt(parent->GM_km3s2 * (2.0f / PE - 1.0f / a_semi));
}

Eigen::Vector3f Orbit::normal() const {
    // Calculate normal vector of orbital plane
    Eigen::Vector3f up(0, 1, 0);
    Eigen::Vector3f solarprimevector(1, 0, 0);

    auto ANdir = Eigen::AngleAxisf(deg2rad(-LAN), up) * solarprimevector;
    auto N = Eigen::AngleAxisf(deg2rad(-inclination), ANdir) * up;
    if (N.y() < 0) { N = -N; }

    return N;
}

float EccentricAnomalySolver(float M, float e) {
    // Solve Kepler's equation: M = E - e * sin(E)
    // Using Newton-Raphson method
    float E = M; // Initial guess
    for (int i = 0; i < 5; ++i) {
        float f = E - e * sin(E) - M;
        float f_prime = 1 - e * cos(E);
        E -= f / f_prime;
    }
    return E;
}

float TrueAnomaly(float E, float e) {
    // Calculate true anomaly from eccentric anomaly
    return 2 * atan2(sqrt(1 + e) * sin(E / 2), sqrt(1 - e) * cos(E / 2));
}

std::pair<vec3f, vec3f> get_local_position(const Orbit& o) {
    float e = o.eccentricity;
    float E0 = e > 0 ? EccentricAnomalySolver(o.meanAnomaly, e) : o.meanAnomaly;
    float phi = TrueAnomaly(E0, e);
    float p = o.a_semi * (1 - e * e);
    float rmag = p / (1 + e * cos(phi));

    vec3f r0;
    vec3f v0;

    r0.x() = rmag * cos(phi);
    r0.y() = 0;
    r0.z() = rmag * sin(phi);

    v0.x() = -sqrt(o.parent->GM_km3s2 / p) * sin(phi);
    v0.y() = 0;
    v0.z() = sqrt(o.parent->GM_km3s2 / p) * (e + cos(phi));

    auto transform = Eigen::AngleAxisf(deg2rad(-o.argumentOfPeriapsis), Eigen::Vector3f::UnitY()) *
                     Eigen::AngleAxisf(deg2rad(-o.inclination), Eigen::Vector3f::UnitX()) *
                     Eigen::AngleAxisf(deg2rad(-o.LAN), Eigen::Vector3f::UnitY());
    r0 = transform * r0;
    v0 = transform * v0;
    return {r0, v0};
}
