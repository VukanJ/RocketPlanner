#ifndef LAMBER_SOLVER_H
#define LAMBER_SOLVER_H

#include "Orbit.h"
#include "kspConstants.h"
#include "utils.h"
#include <cstdint>

class LambertSolver {
public:
    LambertSolver(const Body* ref, Orbit& origin, Orbit& target, std::uint64_t date);

    void solve(float t_departure, float dt);

    vec3f r1; // Position at departure
    vec3f r2; // Position at arrival

    vec3f v1; // Velocity at departure
    vec3f v2; // Velocity at arrival

    std::uint64_t date;
    const Body* refBody;
    Orbit originOrbit;
    Orbit targetOrbit;
};

#endif // LAMBER_SOLVER_H
