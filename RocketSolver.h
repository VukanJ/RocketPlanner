#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include <cmath>
#include <cstdint>
#include <vector>

#include "parts.h"
using PartInfoList = std::vector<const PartProperty*>;

constexpr short MAX_ASPARAGUS_SYMMETRY = 4;
constexpr short MAX_ASPARAGUS_SUBSTAGES = 3;

class RocketSolver {
public:
    RocketSolver(const PartInfoList engines);
    struct AsparagusConfig {
        int baseSymmetry = 0; // 0 = No asparagus
        int numAsparagusStages = 0; // Number of asparagus stages connected to base stages

        // Bitmask indicating which asparagus stages have engines (1=engine, 0=no engine)
        // LSB = innermost asparagus stage
        std::uint16_t hasEngine = 0x0;  // Drop tanks is default
        std::vector<double> fuelFractions;
        std::vector<double> deltaVFractions;
    };

    struct StageInfo {
        double fullMass = INFINITY;
        double emptyMass = INFINITY;
        const PartProperty* engine = nullptr;
        int engineMultiplicity = 0;
        double TWR = 0;
        AsparagusConfig asparagus_config;
    };
    struct RocketConfig {
        std::vector<StageInfo> stages;
        double totalMass = INFINITY;
    };

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=KspSystem::Kerbin.surfaceGravity);

private:
    RocketSolver::RocketConfig buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0=KspSystem::Kerbin.surfaceGravity);
};


#endif // ROCKET_SOLVER_H
