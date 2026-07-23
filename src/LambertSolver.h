#ifndef LAMBER_SOLVER_H
#define LAMBER_SOLVER_H

#include "Orbit.h"
#include "kspConstants.h"
#include "utils.h"

class LambertSolver {
public:
    void init(const Body* ref, const Orbit& origin, const Orbit& target,
              const Body* originBody, const Body* targetBody,
              float startOrbitAltitude, float targetOrbitAltitude);

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
    const Body* originBody = nullptr;
    const Body* targetBody = nullptr;
    Orbit originOrbit;
    Orbit targetOrbit;
    float startOrbitAltitude = 0;
    float targetOrbitAltitude = 0;
};

#endif // LAMBER_SOLVER_H
