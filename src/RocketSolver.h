#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include <cmath>
#include <cstdint>
#include <vector>

#include "parts.h"
#include "RocketConfig.h"

using PartInfoList = std::vector<const PartProperty*>;

constexpr short MAX_ASPARAGUS_SYMMETRY = 4;
constexpr short MAX_ASPARAGUS_SUBSTAGES = 3;

class RocketSolver {
public:
    RocketSolver(const PartInfoList engines);

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=KspSystem::Kerbin.surfaceGravity, double seaLevelAtm=0.0);

private:
    RocketConfig buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0, double seaLevelAtm);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0=KspSystem::Kerbin.surfaceGravity, double atmPressure=0.0);
};


#endif // ROCKET_SOLVER_H
