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
    //struct StageOption {
    //    const PartProperty* main_engine = nullptr;
    //    int engineMultiplicity = 0;
    //    RadialTankConfig radialTankConfig = RadialTankConfig::None;
    //    std::vector<std::pair<const PartProperty*, float>> radialTank_fuelFraction;
    //
    //    float TWR = 0;
    //    float burnTime = 0;
    //    float deltaV = 0;
    //};
    //struct StageVariants {
    //    std::vector<StageOption> options;
    //};

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
        double totalDeltaV = 0;
    };

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=Constants::g0_kerbin);

private:
    RocketSolver::RocketConfig liftoffWeight(const std::vector<double>& deltaVPerStage, double payloadMass);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double g0=Constants::g0_kerbin);
};


#endif // ROCKET_SOLVER_H
