#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include "parts.h"

class RocketSolver {
public:
    enum class RadialTankConfig {
        None,         // No radial tanks, only central tank
        PumpToMiddle, // Outer tanks pump to central tank simultaneously
        Aspargus2,    // 4 -> 2 -> 1 Every substage has two tanks less
        Apargus4,     // 8 -> 4 -> 1 Every substage has four tanks less
    };
    struct StageOption {
        const PartProperty* main_engine = nullptr;
        int engineMultiplicity = 0;
        RadialTankConfig radialTankConfig = RadialTankConfig::None;
        std::vector<std::pair<const PartProperty*, float>> radialTank_fuelFraction;

        float TWR = 0;
        float burnTime = 0;
        float deltaV = 0;
    };
    struct StageVariants {
        std::vector<StageOption> options;
    };

    std::vector<StageVariants> stages;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=Constants::g0_kerbin);
};


#endif // ROCKET_SOLVER_H
