#ifndef ROCKET_SOLVER_H
#define ROCKET_SOLVER_H

#include <cmath>
#include <cstdint>
#include <vector>

#include "parts.h"
using PartInfoList = std::vector<const PartProperty*>;

template <typename FLT>
struct FlightData {
    std::vector<FLT> t;
    std::vector<FLT> altitude_km;
    std::vector<FLT> velocity_ms;
    std::vector<FLT> vx_ms;
    std::vector<FLT> vy_ms;
    std::vector<FLT> posx_km;
    std::vector<FLT> posy_km;
    std::vector<FLT> thrust_kN;
    std::vector<FLT> mass_t;
    std::vector<FLT> pressure_atm;
    std::vector<FLT> dir_angle_deg;
    std::vector<FLT> apoapsis_km;
    std::vector<int> stage;
    std::vector<FLT> area_m2;
    std::vector<FLT> drag_N;
};

constexpr short MAX_ASPARAGUS_SYMMETRY = 4;
constexpr short MAX_ASPARAGUS_SUBSTAGES = 3;

struct StageKinematics {
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
        int baseSymmetry = 1; // 1 = No asparagus
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
        const PartProperty* boosterEngine = nullptr;  // Radial engines
        int engineMultiplicity = 0;
        double TWR = 0;
        AsparagusConfig asparagus_config;
    };
    struct RocketConfig {
        std::vector<StageInfo> stages;
        void calcStageKinematics(std::vector<StageKinematics>& kinematics) const;
        void recomputeMasses(const std::vector<double>& fuelMass);
        int totalStages() const;
        double totalMass = INFINITY;
    };

    PartInfoList allEngines;

    void solve(double targetDeltaV, double payloadMass, double minTWR=0, double g0=KspSystem::Kerbin.surfaceGravity, double seaLevelAtm=0.0);

private:
    RocketSolver::RocketConfig buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0, double seaLevelAtm);
    StageInfo solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0=KspSystem::Kerbin.surfaceGravity, double atmPressure=0.0);
};


FlightData<float> simulate_flight(Body body, const RocketSolver::RocketConfig& rocket);

#endif // ROCKET_SOLVER_H
