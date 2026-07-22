#include "Orbit.h"
#include "DeltaV.h"
#include "helper.h"
#include "kspConstants.h"
#include "eigen3/Eigen/Geometry"
#include "utils.h"

#include <cmath>

Orbit Orbit::circular(const Body *ref, float R) {
    Orbit o;
    o.parent = ref;
    o.AP = R;
    o.PE = R;
    o.a_semi = R;
    return o;
}

Orbit Orbit::elliptic(const Body* ref, float ap, float pe) {
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

Orbit Orbit::fromLambert(const Body* ref, vec3f r1, vec3f r2, vec3f Fprime, float a) {
    // Initialize an orbit and all its Keplerian orbital elements 
    // given two positions on the orbit, r1 & r2, the hidden focal point Fprime and the semimajor axis.
    // It is assumed that the sun is at the origin
    Orbit orbit;

    const float e = Fprime.norm() / (2.0f * a);

    // Orbital plane normal (ensure prograde: n.y >= 0)
    vec3f n = r1.cross(r2).normalized();
    if (n.y() < 0.0f) { n = -n; }

    orbit.parent = ref;
    orbit.eccentricity = e;
    orbit.a_semi = a;
    orbit.AP = a * (1.0 + e);
    orbit.PE = a * (1.0 - e);
    orbit.inclination = acos(n.y()) * 180.0f / M_PI;

    orbit.LAN = atan2(n.x(), -n.z()) * 180.0f / M_PI;
    orbit.LDN = fmod(orbit.LAN + 180.0f, 360.0f);

    // Argument of periapsis
    float lan_rad = orbit.LAN * M_PI / 180.0f;
    vec3f AN_hat(cos(lan_rad), 0.0f, sin(lan_rad));
    vec3f e2 = AN_hat.cross(n);

    // Periapsis position vector from focus at origin
    vec3f r_p = (e - 1.0f) / (2.0f * e) * Fprime;
    vec3f r_p_hat = r_p.normalized();

    float cosw = r_p_hat.dot(AN_hat);
    float sinw = r_p_hat.dot(e2);
    orbit.argumentOfPeriapsis = atan2(sinw, cosw) * 180.0f / M_PI;

    // True anomaly at r1
    vec3f r1_hat = r1.normalized();
    float cosnu = r1_hat.dot(r_p_hat);
    float sinnu = r1_hat.dot(n.cross(r_p_hat));
    float nu = atan2(sinnu, cosnu);

    // Eccentric anomaly (numerically stable atan2 form)
    float E = 2.0f * atan2(sqrt(1.0f - e) * sin(nu / 2.0f),
                           sqrt(1.0f + e) * cos(nu / 2.0f));

    // Mean anomaly (Kepler's equation)
    orbit.meanAnomaly = E - e * sin(E);
    if (orbit.meanAnomaly < 0.0f) { orbit.meanAnomaly += 2.0f * M_PI; }
    if (orbit.meanAnomaly >= 2.0f * M_PI) { orbit.meanAnomaly -= 2.0f * M_PI; }

    return orbit;
}

inline constexpr float deg2rad(float f) {
    return f * M_PI / 180.0f;
}

float Orbit::v_apoapsis(float unit) const {
    return unit * std::sqrt(parent->GM_km3s2 * (2.0f / AP - 1.0f / a_semi));
}

float Orbit::v_periapsis(float unit) const {
    return unit * std::sqrt(parent->GM_km3s2 * (2.0f / PE - 1.0f / a_semi));
}

float Orbit::period(float unit) const {
    return 2.0f * M_PI * std::sqrt(a_semi * a_semi * a_semi / parent->GM_km3s2) * unit;
}

float Orbit::trueAnomalyAt(const vec3f& r) const {
    Eigen::Matrix3f transform =
        Eigen::AngleAxisf(deg2rad(-LAN), Eigen::Vector3f::UnitY()).toRotationMatrix() *
        Eigen::AngleAxisf(deg2rad(-inclination), Eigen::Vector3f::UnitX()).toRotationMatrix() *
        Eigen::AngleAxisf(deg2rad(-argumentOfPeriapsis), Eigen::Vector3f::UnitY()).toRotationMatrix();
    vec3f r_perifocal = transform.transpose() * r;
    return std::atan2(r_perifocal.z(), r_perifocal.x());
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

    auto transform = Eigen::AngleAxisf(deg2rad(-o.LAN), Eigen::Vector3f::UnitY()) *
                     Eigen::AngleAxisf(deg2rad(-o.inclination), Eigen::Vector3f::UnitX()) *
                     Eigen::AngleAxisf(deg2rad(-o.argumentOfPeriapsis), Eigen::Vector3f::UnitY());
    r0 = transform * r0;
    v0 = transform * v0;
    return {r0, v0};
}

std::pair<vec3f, vec3f> get_local_future_position(const Orbit& o, std::uint64_t seconds) {
    // Compute true anomaly at a future time
    float e = o.eccentricity;
    float M = fmod(o.meanAnomaly + 2.0f * M_PI * (seconds / o.period()), 2.0f * M_PI);
    float E0 = e > 0 ? EccentricAnomalySolver(o.meanAnomaly, e) : o.meanAnomaly;
    float Et = e > 0 ? EccentricAnomalySolver(M, e) : M;

    auto [r0, v0] = get_local_position(o);

    float r0mag = r0.norm();

    vec3f r_t;
    vec3f v_t;

    r_t = o.a_semi / r0mag * (cos(Et-E0) - e * cos(E0)) * r0 + o.period() / (2.0 * M_PI) * (sin(Et-E0) - e * (sin(Et) - sin(E0))) * v0;
    float rtmag = r_t.norm();
    v_t = -std::sqrt(o.parent->GM_km3s2 * o.a_semi) / (rtmag * r0mag) * sin(Et-E0) * r0 + o.a_semi / rtmag * (cos(Et-E0) - e * cos(Et)) * v0;

    return {r_t, v_t};
}

float default_transfer_time_estimate(const Body* from, const Body* to) {
    // Calculate the default transfer time between two bodies using Hohmann transfer assumption
    // Used to estimate the time window for interplanetary transfers
    assert(from->orbit.parent == to->orbit.parent);
    auto o = getHohmannOrbit(from->orbit.parent, from->orbit.AP, to->orbit.PE);

    auto t1 = o.period() / 2.0f;
    o = getHohmannOrbit(from->orbit.parent, from->orbit.PE, to->orbit.AP);
    auto t2 = o.period() / 2.0f;

    return 0.5f * (t1 + t2);
}

float default_transfer_window_estimate(const Body* from, const Body* to) {
    // Compute minimum time window estimate for hohmann transfer
    float T1 = from->orbit.period(unit_day);
    float T2 = to->orbit.period(unit_day);

    float dTheta = std::abs(2.0f * M_PI * (1.0f / T2 - 1.0f / T1));

    return 2.0f * M_PI / dTheta / unit_day;
}
