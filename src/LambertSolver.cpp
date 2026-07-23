#include "LambertSolver.h"
#include "DeltaV.h"
#include <cmath>
#include <Eigen/Geometry>

void LambertSolver::init(const Body* ref, const Orbit& origin, const Orbit& target,
                         const Body* originBody_, const Body* targetBody_,
                         float startAltitude, float targetAltitude) {
    refBody = ref;
    originOrbit = origin;
    targetOrbit = target;
    originBody = originBody_;
    targetBody = targetBody_;
    startOrbitAltitude = startAltitude;
    targetOrbitAltitude = targetAltitude;
}

float Lambert(float s, float c, float a, float mu) {
    float alpha = 2.0f * asin(sqrt(s / (2.0f * a)));
    float beta = 2.0f * asin(sqrt((s - c) / (2.0f * a)));
    return sqrt(a * a * a / mu) * (alpha - sin(alpha) - beta + sin(beta));
}

bool LambertSolver::solve(float t_departure, float dt) {
    if (dt <= 0.0f) {
        return false;
    }

    // Get initial orbital position and position of target after given flight time dt
    auto [r1_local, v1_local] = get_local_future_position(originOrbit, t_departure);
    r1 = r1_local;
    auto [r2_local, v2_local] = get_local_future_position(targetOrbit, t_departure + dt);
    r2 = r2_local;

    const float mu = originOrbit.parent->GM_km3s2;
    const float c = (r2 - r1).norm();  // chord
    if (c <= 0.0f) {
        return false;
    }
    const float s = 0.5f * (r1.norm() + r2.norm() + c);  // Semi-perimeter
    const float amin = 0.5f * s;  // minimum energy semimajor axis
    float amax = 2*s;

    // Find amax so that its travel time is below dt (T(a) is decreasing)
    for (int i = 0; i < 64 && Lambert(s, c, amax, mu) > dt; ++i) {
        amax *= 2;
    }
    if (Lambert(s, c, amax, mu) > dt) {
        return false;
    }

    // Bisection algorithm
    float A = amin;
    float B = amax;
    float a = 0.5f * (A + B);
    for (int i = 0; i < 50; ++i) {
        float dt_l = Lambert(s, c, a, mu);
        if (dt_l > dt) {
            A = a;
        }
        else {
            B = a;
        }
        a = 0.5f * (A + B);
    }
    // Done
    
    // Construct the two possible focal points by circle intersection
    const float R1 = 2.0f * a - r1.norm();
    const float R2 = 2.0f * a - r2.norm();
    const float d = (r2 - r1).norm();
    vec3f d_hat = (r2 - r1) / d;

    float a_param = (R1 * R1 - R2 * R2 + d * d) / (2.0f * d);
    float h = std::sqrt(std::max(R1 * R1 - a_param * a_param, 0.0f));

    vec3f m = r1 + a_param * d_hat;
    vec3f orbitalNormal = r1.cross(r2);
    if (orbitalNormal.squaredNorm() == 0.0f) {
        return false;
    }
    vec3f n = orbitalNormal.normalized();
    vec3f perp = n.cross(d_hat);
    vec3f focal1 = m + h * perp;
    vec3f focal2 = m - h * perp;

    // Reconstruct both transfer orbits
    transferOrbit1 = Orbit::fromLambert(refBody, r1, r2, focal1, a);
    transferOrbit2 = Orbit::fromLambert(refBody, r1, r2, focal2, a);

    // Get transfer orbit velocities at r1 and r2
    auto [r1_check1, v1t1] = get_local_position(transferOrbit1);
    auto [r2_check1, v2t1] = get_local_future_position(transferOrbit1, dt);
    auto [r1_check2, v1t2] = get_local_position(transferOrbit2);
    auto [r2_check2, v2t2] = get_local_future_position(transferOrbit2, dt);

    // Departure velocities on the original orbits
    auto [r1_orig, v1_orig] = get_local_future_position(originOrbit, t_departure);
    auto [r2_orig, v2_orig] = get_local_future_position(targetOrbit, t_departure + dt);

    deltaV1_depart = orbitalInsertion(originBody, v1_orig, v1t1, startOrbitAltitude);
    deltaV1_arrive = circularizeHyperbolicCost(targetBody, v2_orig, v2t1, targetOrbitAltitude);
    deltaV2_depart = orbitalInsertion(originBody, v1_orig, v1t2, startOrbitAltitude);
    deltaV2_arrive = circularizeHyperbolicCost(targetBody, v2_orig, v2t2, targetOrbitAltitude);
    return true;
}
