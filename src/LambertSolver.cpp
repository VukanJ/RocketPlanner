#include "LambertSolver.h"
#include <cmath>
#include <Eigen/Geometry>

void LambertSolver::init(const Body* ref, const Orbit& origin, const Orbit& target, std::uint64_t date_seconds) {
    date = date_seconds;
    refBody = ref;
    originOrbit = origin;
    targetOrbit = target;
}

float Lambert(float s, float c, float a, float mu) {
    float alpha = 2.0f * asin(sqrt(s / (2.0f * a)));
    float beta = 2.0f * asin(sqrt((s - c) / (2.0f * a)));
    return sqrt(a * a * a / mu) * (alpha - sin(alpha) - beta + sin(beta));
}

void LambertSolver::solve(float t_departure, float dt) {
    // Get initial orbital position and position of target after given flight time dt
    auto [r1_local, v1_local] = get_local_future_position(originOrbit, date + t_departure);
    r1 = r1_local;
    auto [r2_local, v2_local] = get_local_future_position(targetOrbit, date + t_departure + dt);
    r2 = r2_local;

    const float mu = originOrbit.parent->GM_km3s2;
    const float c = (r2 - r1).norm();  // chord
    const float s = 0.5f * (r1.norm() + r2.norm() + c);  // Semi-perimeter
    const float amin = 0.5f * s;  // minimum energy semimajor axis
    float amax = 2*s;

    // Find amax so that its travel time is below dt (T(a) is decreasing)
    while (Lambert(s, c, amax, mu) > dt) {
        amax *= 2;
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
    
    // Compute transfer orbit velocities via Lagrange coefficients
}
