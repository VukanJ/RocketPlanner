#include "DeltaV.h"

#include <cmath>
#include "utils.h"
#include "eigen3/Eigen/Geometry"

float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm) {
    float alpha = body->GM();
    float R = body->radius_km;
    float Rtarget = body->radius_km + targetAltitudeKm;
    float a = 0.5f * (R + Rtarget);

    float dv = 1000.0f * (std::sqrt(alpha * (2.0f / R - 1.0f / a))
                        - std::sqrt(alpha * (2.0f / Rtarget - 1.0f / a))
                        + std::sqrt(alpha / Rtarget));

    if (body->hasAtmosphere()) {
        // Assume rocket gains altitude at terminal velocity while ascending by the atmospheric scaling height.
        // The time spent in the atmosphere can be translated into a gravity loss expression that scales
        // well to all bodies in the KSP system with an atmosphere.
        //
        // v_terminal ∝ sqrt(g / ρ).
        // Calculate time needed to pass one scale height at terminal velocity
        // t_climb = H / v_term
        // Finally, the total accumulated gravity Δv loss during that time is g0 * t_climb
        // This is very crude, but after adding a scale parameter k_atm=50.0f tuned to Kerbins known value of ~3500 m/s 
        // The formula happens to match the published dV maps quite well
        constexpr float k_atm = 50.0f;
        dv += k_atm * std::sqrt(body->surfaceGravity)
                    * body->atm_falloff_km
                    * std::sqrt(body->seaLevel_atm);
    }

    return dv;
}

float escapeBurnCost(const Body* origin, float startAltitude) {
    float v_circ = unit_mps * std::sqrt(origin->GM() / (origin->radius_km + startAltitude));
    return v_circ * (std::numbers::sqrt2 - 1.0);
}

Orbit getHohmannOrbit(const Body* origin, const Body* target, float startAltitude) {
    // Assume orbits are coplanar and orbit A is circular
    if (origin != target->orbit.parent) {
        throw std::runtime_error("Hohmann transfer must different reference bodies!");
    }

    Orbit he;
    he.PE = startAltitude + origin->radius_km;
    he.AP =  target->orbit.AP;

    float a = 0.5f * (he.PE + he.AP);
    float b = std::sqrt(he.PE * he.AP);
    he.eccentricity = std::sqrt(1.0 - b*b / (a*a));
    he.a_semi = a;
    he.parent = origin;
    return he;
}

float hohmannTransferCost(const Body* origin, const Body* target, float startAltitude) {
    // Calculate dV range for hohmann transfer
    Orbit o_target = target->orbit;
    if (origin != target->orbit.parent) {
        // This is not a Hohmann transfer
        return -1;
    }

    Orbit o_origin = Orbit::circular(origin, startAltitude + origin->radius_km);

    if ((o_origin.PE > o_target.AP && o_origin.AP < o_target.PE) || (o_target.PE > o_origin.AP && o_target.AP < o_origin.PE)) {
        // Orbits intersect
        return -4;
    }

    // Coplanar orbits, one contains the other. Starting orbit is circular
    Orbit hohmann = getHohmannOrbit(origin, target, startAltitude);

    float dV = hohmann.v_periapsis(unit_mps) - o_origin.v_apoapsis(unit_mps);
    return dV;
}

float circularizeHyperbolicCost(const Body* origin, const Body* target, float startAltitude, float rmin, bool prograde) {
    // Compute speed of target body on its orbit
    float vTarget = target->orbit.v_apoapsis(unit_kmps);
    rmin += target->radius_km;

    Orbit hohmann = getHohmannOrbit(origin, target, startAltitude);

    float v0 = 0;
    if (prograde) {
        v0 = vTarget - hohmann.v_apoapsis(unit_kmps);
    }
    else {
        v0 = vTarget + hohmann.v_apoapsis(unit_kmps);
    }
    float alpha = target->GM();
    float eps = 0.5f * v0 * v0 - alpha / target->R_SOI_km;
    float vmax = std::sqrt(2.0f * (eps + alpha / rmin));  // Vis-viva

    float dV = unit_mps * std::abs(vmax - std::sqrt(alpha / rmin));
    return dV;
}

float directionToAnomaly(const Eigen::Vector3f& dir, const Orbit& orbit) {
    // Given a direction, return the anomaly of the point this vector is pointing at on the orbit
    Eigen::Vector3f up(0, 1, 0);
    Eigen::Vector3f solarprimevector(1, 0, 0);
    
    auto ANdir = Eigen::AngleAxisf(deg2rad(-orbit.LAN), up) * solarprimevector;
    auto N = orbit.normal();

    auto dirPER = Eigen::AngleAxisf(deg2rad(-orbit.argumentOfPeriapsis), N) * ANdir;

    // Project direction onto orbital plane
    Eigen::Vector3f dirProj = dir - N * dir.dot(N);
    dirProj.normalize();

    float anomaly = std::acos(std::clamp(dirProj.dot(dirPER), -1.0f, 1.0f));

    // If direction is past apoapsis (approaching periapsis), anomaly > 180
    if (N.cross(dirPER).dot(dirProj) > 0) {
        anomaly = 2.0f * M_PI - anomaly;
    }

    return anomaly;
}

float inclinationCorrectionCost(const Orbit& origin, const Orbit& target) {
    Eigen::Vector3f up(0, 1, 0);
    Eigen::Vector3f solarprimevector(1, 0, 0);  // reference vector

    auto ANdir_target = Eigen::AngleAxisf(deg2rad(-target.LAN), up) * solarprimevector;
    auto ANdir_origin = Eigen::AngleAxisf(deg2rad(-origin.LAN), up) * solarprimevector;

    // Get normals of orbital planes
    // By rotating up vector by inclination around the AN directions
    auto N_target = Eigen::AngleAxisf(deg2rad(-target.inclination), ANdir_target) * up;
    auto N_origin = Eigen::AngleAxisf(deg2rad(-origin.inclination), ANdir_origin) * up;

    if (N_target.y() < 0) {
        N_target = -N_target;
    }
    if (N_origin.y() < 0) {
        N_origin = -N_origin;
    }

    // Relative inclination angle between the two orbital planes
    float relInclination = std::acos(std::clamp(N_origin.dot(N_target), -1.0f, 1.0f));

    // If planes are already aligned, no cost
    if (relInclination < 1e-6f) { return 0; }

    // Line of nodes (intersection of the two planes) — both directions
    auto nodeDir = N_origin.cross(N_target).normalized();

    // True anomaly of each node on the origin orbit
    float theta_a = directionToAnomaly(nodeDir, origin);
    float theta_b = directionToAnomaly(-nodeDir, origin);

    float mu = origin.parent->GM();
    float a = origin.a_semi;
    float e = origin.eccentricity;

    auto speedAt = [&](float theta) {
        float cosT = std::cos(theta);
        float r = a * (1.0f - e * e) / (1.0f + e * cosT);
        return std::sqrt(mu * (2.0f / r - 1.0f / a));
    };

    float v_a = speedAt(theta_a);
    float v_b = speedAt(theta_b);

    // dV = 2 * v * sin(Δi/2) for a pure plane-change burn at the node
    float dV_a = 2.0f * v_a * std::sin(relInclination / 2.0f);
    float dV_b = 2.0f * v_b * std::sin(relInclination / 2.0f);

    // Return the cheaper of the two nodes (convert km/s → m/s)
    return unit_mps * std::min(dV_a, dV_b);
}

