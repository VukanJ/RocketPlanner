#include "DeltaV.h"

#include <cmath>
#include <numbers>
#include "utils.h"
#include "eigen3/Eigen/Geometry"

float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm) {
    float GM = body->GM();
    float R = body->radius_km;
    float Rtarget = body->radius_km + targetAltitudeKm;
    float a = 0.5f * (R + Rtarget);

    float dv = unit_mps * (std::sqrt(GM * (2.0f / R - 1.0f / a))
                        - std::sqrt(GM * (2.0f / Rtarget - 1.0f / a))
                        + std::sqrt(GM / Rtarget));

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

Orbit getHohmannOrbit(const Body* center, float R1, float R2) {
    Orbit he;
    if (R1 > R2) {
        std::swap(R1, R2);
    }
    he.PE = R1;
    he.AP = R2;

    float a = 0.5f * (he.PE + he.AP);
    float b = std::sqrt(he.PE * he.AP);
    he.eccentricity = std::sqrt(1.0 - b*b / (a*a));
    he.a_semi = a;
    he.parent = center;
    return he;
}

DeltaVRange hohmannTransferCost(const Body* origin, const Body* target, float startAltitude) {
    const Body* center = nullptr;
    float R1[2];

    if (origin == target->orbit.parent) {
        // Planet -> moon
        center = origin;
        R1[0] = R1[1] = startAltitude + origin->radius_km;
    }
    else if (origin->orbit.parent == target->orbit.parent) {
        // Planet -> planet
        center = origin->orbit.parent;
        R1[0] = origin->orbit.PE;
        R1[1] = origin->orbit.AP;
    }
    else if (target == origin->orbit.parent) {
        // Moon -> planet: escape burn already sets up the transfer
        return DeltaVRange(0);
    }
    else {
        return { };
    }

    float GM = center->GM();
    float R2[2] = { (float)target->orbit.PE, (float)target->orbit.AP };

    DeltaVRange dv_range;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            Orbit hohmann = getHohmannOrbit(center, R1[i], R2[j]);
            float v_depart = std::sqrt(GM * (2.0f / R1[i] - 1.0f / hohmann.a_semi));
            float v_circular = std::sqrt(GM / R1[i]);
            dv_range.add(unit_mps * std::abs(v_depart - v_circular));
        }
    }
    return dv_range;
}

DeltaVRange circularizeHyperbolicCost(const Body* origin, const Body* target, float startAltitude, float rmin, bool prograde) {
    rmin += target->radius_km;
    float alpha = target->GM();
    float vCircular = std::sqrt(alpha / rmin);

    auto computeCost = [&](float v_transfer, float vTarget) -> float {
        float v0 = prograde ? (vTarget - v_transfer) : (vTarget + v_transfer);
        float eps = 0.5f * v0 * v0 - alpha / target->R_SOI_km;
        float vmax = std::sqrt(2.0f * (eps + alpha / rmin));
        return unit_mps * std::abs(vmax - vCircular);
    };

    DeltaVRange dv_range;

    if (origin->orbit.parent == target->orbit.parent) {
        // Planet -> planet: sweep all four (R1, R2) combinations
        const Body* center = origin->orbit.parent;
        float GM = center->GM();
        for (float R1 : {origin->orbit.PE, origin->orbit.AP}) {
            for (float R2 : {target->orbit.PE, target->orbit.AP}) {
                Orbit hohmann = getHohmannOrbit(center, R1, R2);
                float v_transfer = std::sqrt(GM * (2.0f / R2 - 1.0f / hohmann.a_semi));
                float vTarget    = std::sqrt(GM * (2.0f / R2 - 1.0f / target->orbit.a_semi));
                dv_range.add(computeCost(v_transfer, vTarget));
            }
        }
    }
    else if (origin == target->orbit.parent) {
        // Planet -> moon: fixed departure, sweep target PE/AP
        float R1 = startAltitude + origin->radius_km;
        float GM = origin->GM();
        for (float R2 : {target->orbit.PE, target->orbit.AP}) {
            Orbit hohmann = getHohmannOrbit(origin, R1, R2);
            float v_transfer = std::sqrt(GM * (2.0f / R2 - 1.0f / hohmann.a_semi));
            float vTarget    = std::sqrt(GM * (2.0f / R2 - 1.0f / target->orbit.a_semi));
            dv_range.add(computeCost(v_transfer, vTarget));
        }
    }

    return dv_range;
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

DeltaVRange inclinationCorrectionCost(const Orbit& origin, const Orbit& target) {
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
    if (relInclination < 0.001) { return 0; }

    // Line of nodes (intersection of the two planes) — both directions
    auto nodeDir = N_origin.cross(N_target).normalized();

    // True anomaly of each node on the origin orbit
    float theta_a = directionToAnomaly(nodeDir, origin);
    float theta_b = directionToAnomaly(-nodeDir, origin);

    float alpha = origin.parent->GM();
    float a = origin.a_semi;
    float e = origin.eccentricity;

    auto speedAt = [&](float theta) {
        float cosT = std::cos(theta);
        float r = a * (1.0f - e * e) / (1.0f + e * cosT);
        return std::sqrt(alpha * (2.0f / r - 1.0f / a));
    };

    float v_a = speedAt(theta_a);
    float v_b = speedAt(theta_b);

    // dV = 2 * v * sin(Δi/2) for a pure plane-change burn at the node
    float dV_a = 2.0f * v_a * std::sin(relInclination / 2.0f);
    float dV_b = 2.0f * v_b * std::sin(relInclination / 2.0f);

    // Return the cheaper of the two nodes (convert km/s → m/s)
    DeltaVRange dv_range;
    dv_range.add(unit_mps * dV_a);
    dv_range.add(unit_mps * dV_b);
    return dv_range;
}

DeltaVRange orbitalInsertion(const Body* origin, const Body* target, float startAltitude) {
    DeltaVRange v_infinity = hohmannTransferCost(origin, target, 0);
    float v_circ = unit_mps * std::sqrt(origin->GM() / (startAltitude + origin->radius_km));
    float v_escape = std::numbers::sqrt2 * v_circ;

    DeltaVRange dv_range;
    dv_range.add(std::abs(std::sqrt(v_infinity.max * v_infinity.max + v_escape * v_escape) - v_circ));
    dv_range.add(std::abs(std::sqrt(v_infinity.min * v_infinity.min + v_escape * v_escape) - v_circ));
    return dv_range;
}

