#ifndef ROCKET_CONFIG_H
#define ROCKET_CONFIG_H

#include <cmath>

#include "parts.h"

// Abstract kinematic rocket description

struct StageKinematics {
    float m0; // mass of rocket at stage ignition
    float mf; // mass of rocket at stage burnout
    float burnTime;
    float area_m2;
    float vacuumDeltaV;
    const PartProperty* engine;
    int nEngines;
};

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
    float fullMass = INFINITY;
    float payloadMass = 0;  // Dead weight in this stage
    float emptyMass = INFINITY;
    const PartProperty* engine = nullptr;
    const PartProperty* boosterEngine = nullptr;  // Radial engines
    int engineMultiplicity = 0;
    float TWR = 0;
    AsparagusConfig asparagus_config;
};

class RocketConfig {
public:
    void calcStageKinematics(std::vector<StageKinematics>& kinematics) const;
    void recomputeMasses(const std::vector<double>& fuelMass);
    int totalStages() const;
    double remainingDeltaV(const std::vector<StageKinematics>& kinematics, float elapsed) const;

    std::vector<StageInfo> stages;
    double totalMass = INFINITY;
};

#endif // ROCKET_CONFIG_H
