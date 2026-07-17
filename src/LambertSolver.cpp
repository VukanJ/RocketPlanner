#include "LambertSolver.h"
#include <cmath>

LambertSolver::LambertSolver(const Body* ref, Orbit& origin, Orbit& target, std::uint64_t date_seconds) 
    : date(date_seconds), 
      refBody(ref), 
      originOrbit(origin), 
      targetOrbit(target)
{ }

float Lambert(float s, float c, float a, float T) {
    float alpha = acos(1 - c / a);
    float beta = acos(1 - s / a);
    return T * (alpha - beta - sin(alpha) + sin(beta));
}

void LambertSolver::solve(float t_departure, float dt) {
    // Get initial orbital position and position of target after given flight time dt
    auto [r1_local, v1_local] = get_local_future_position(originOrbit, date + t_departure);
    r1 = r1_local;
    v1 = v1_local;
    auto [r2_local, v2_local] = get_local_future_position(targetOrbit, date + t_departure + dt);
    r2 = r2_local;
    v2 = v2_local;

    const float c = (r2 - r1).norm();  // chord
    const float s = 0.5f * (r1.norm() + r2.norm() + c);  // Semi-perimeter
    const float amin = 0.5f * s;  // minimum energy semimajor axis
    const float T2pi = std::sqrt(amin * amin * amin / originOrbit.parent->GM_km3s2);
    const float t_amin = Lambert(s, c, amin, T2pi);
    // t_amin needed to estimate solution branch

    // Find a so that Lambert returns dt
    
    // Compute necessary orbital elements
}
