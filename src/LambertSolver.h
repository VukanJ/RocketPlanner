#ifndef LAMBER_SOLVER_H
#define LAMBER_SOLVER_H

#include "Orbit.h"
#include "kspConstants.h"
#include "utils.h"
#include <cstdint>

class LambertSolver {
public:
    void init(const Body* ref, const Orbit& origin, const Orbit& target, std::uint64_t date);

    void solve(float t_departure, float dt);

    vec3f r1; // Position at departure
    vec3f r2; // Position at arrival

    vec3f v1; // Velocity at departure
    vec3f v2; // Velocity at arrival

    std::uint64_t date = 0;
    const Body* refBody = nullptr;
    Orbit originOrbit;
    Orbit targetOrbit;
};

#endif // LAMBER_SOLVER_H
