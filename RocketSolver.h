#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include <cmath>
#include <cstdint>
#include <vector>

#include "parts.h"
using PartInfoList = std::vector<const PartProperty*>;

constexpr short MAX_ASPARAGUS_SYMMETRY = 4;
constexpr short MAX_ASPARAGUS_SUBSTAGES = 3;

struct StageKinematics {
    // Store basic kinematic info about a rocket stage, 
    // which can be used for numerical integration of the rocket's trajectory.
    float m0;
    float mf;
    float burnTime;
    float area_m2;
    const PartProperty* engine;
    int nEngines;
};

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
        void calcStageKinematics(std::vector<StageKinematics>& kinematics) const;
        int totalStages() const;
        double totalMass = INFINITY;
    };

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=KspSystem::Kerbin.surfaceGravity, double seaLevelAtm=0.0);

private:
    RocketSolver::RocketConfig buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0, double seaLevelAtm);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0=KspSystem::Kerbin.surfaceGravity, double atmPressure=0.0);
};


#endif // ROCKET_SOLVER_H
