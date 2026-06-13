#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include <cmath>
#include <vector>

#include "parts.h"
using PartInfoList = std::vector<const PartProperty*>;

class RocketSolver {
public:
    RocketSolver(const PartInfoList engines);
    enum class RadialTankConfig {
        None,         // No radial tanks, only central tank
        PumpToMiddle, // Outer tanks pump to central tank simultaneously
        Aspargus2,    // 4 -> 2 -> 1 Every substage has two tanks less
        Apargus4,     // 8 -> 4 -> 1 Every substage has four tanks less
    };

    struct StageInfo {
        double fullMass = INFINITY;
        double emptyMass = INFINITY;
        const PartProperty* engine = nullptr;
        int engineMultiplicity = 0;
        double TWR = 0;
    };
    struct RocketConfig {
        std::vector<StageInfo> stages;
        double totalMass = INFINITY;
    };

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=Constants::g0_kerbin);

private:
    RocketSolver::RocketConfig buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0=Constants::g0_kerbin);
};


#endif // ROCKET_SOLVER_H
