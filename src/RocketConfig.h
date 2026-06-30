#ifndef ROCKET_CONFIG_H
#define ROCKET_CONFIG_H

#include <cmath>

#include "parts.h"

// Abstract kinematic rocket description

struct StageKinematics {
    float m0;
    float mf;
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
    double remainingDeltaV(const std::vector<StageKinematics>& kinematics, float elapsed) const;
    double totalMass = INFINITY;
};

#endif // ROCKET_CONFIG_H
