#ifndef LAMBER_SOLVER_H
#define LAMBER_SOLVER_H

#include "Orbit.h"
#include "kspConstants.h"
#include "utils.h"
#include <cstdint>

class LambertSolver {
public:
    void init(const Body* ref, const Orbit& origin, const Orbit& target);

    bool solve(float t_departure, float dt);

    vec3f r1; // Position at departure
    vec3f r2; // Position at arrival

    vec3f v1; // Velocity at departure
    vec3f v2; // Velocity at arrival

    Orbit transferOrbit1;
    Orbit transferOrbit2;
    float deltaV1_depart = 0;
    float deltaV1_arrive = 0;
    float deltaV2_depart = 0;
    float deltaV2_arrive = 0;

    const Body* refBody = nullptr;
    Orbit originOrbit;
    Orbit targetOrbit;
};

#endif // LAMBER_SOLVER_H
