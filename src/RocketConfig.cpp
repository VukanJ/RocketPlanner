#include "RocketConfig.h"

int RocketConfig::totalStages() const {
    int tot = 0;
    for (const auto& stage : stages) {
        tot += 1 + stage.asparagus_config.numAsparagusStages;
    }
    return tot;
}

void RocketConfig::recomputeMasses(const std::vector<double>& fuelMass) {
    double belowMass = 0.0;
    for (int i = stages.size() - 1; i >= 0; --i) {
        auto& stage = stages[i];
        double enginesMass = 0.0;
        if (stage.engine) {
            enginesMass = stage.engine->getMass() * stage.engineMultiplicity;
            auto& asp = stage.asparagus_config;
            if (asp.baseSymmetry > 0 && asp.numAsparagusStages > 0) {
                int boosters = std::popcount((unsigned int)asp.hasEngine) * asp.baseSymmetry;
                enginesMass += stage.engine->getMass() * boosters;
            }
        }
        stage.emptyMass = enginesMass + fuelMass[i] / 9.0 + belowMass;
        stage.fullMass = stage.emptyMass + fuelMass[i];
        if (stage.asparagus_config.baseSymmetry == 0) {
            stage.asparagus_config.fuelFractions = {1.0};
        }
        else if (stage.asparagus_config.fuelFractions.size() != stage.asparagus_config.numAsparagusStages + 1) {
            stage.asparagus_config.fuelFractions.assign(stage.asparagus_config.numAsparagusStages + 1, 1.0 / (stage.asparagus_config.numAsparagusStages + 1));
        }
        belowMass = stage.fullMass;
    }
}

void RocketConfig::calcStageKinematics(std::vector<StageKinematics>& kinematics) const {
    // Compute staging sequences of the rocket 
    // and calculate initial and final masses of that stage, as well as 
    // observables independent of atmospheric flight
    const int totStages = totalStages();
    kinematics.resize(totStages);

    int sptr = 0;
    for (std::size_t stage = 0; stage < stages.size(); ++stage) {
        const auto& S = stages[stage];
        const double totalFuel_tons = S.fullMass - S.emptyMass;
        double currMassTons = S.fullMass;
        int symmetryFac = S.asparagus_config.baseSymmetry;
        symmetryFac = symmetryFac > 1 ? symmetryFac : 0;
        const int asparagus = S.asparagus_config.numAsparagusStages;

        int nEngines = S.engineMultiplicity + std::popcount(S.asparagus_config.hasEngine) * symmetryFac;

        for (int sub = 0; sub < asparagus + 1; ++sub) {
            kinematics[sptr].engine = S.engine;
            kinematics[sptr].nEngines = nEngines;
            int n_radial_attached = symmetryFac * asparagus - sub * symmetryFac;
            kinematics[sptr].area_m2 = Constants::mk2_area_m2 + Constants::mk1_area_m2 * n_radial_attached;
            double burnedFuel = S.asparagus_config.fuelFractions[sub] * totalFuel_tons;
            double tankWeight = burnedFuel / 9.0;
            int detachedEngines = ((S.asparagus_config.hasEngine >> sub) & 0x1) * symmetryFac;
            kinematics[sptr].m0 = currMassTons;
            kinematics[sptr].mf = currMassTons - burnedFuel;
            kinematics[sptr].burnTime = (burnedFuel / S.engine->usedFuelDensity()) / (S.engine->enginePerf.fuelConsumptionRate_UPS * nEngines);
            kinematics[sptr].vacuumDeltaV = S.engine->enginePerf.vacuumISP * 9.81f
                * std::log(kinematics[sptr].m0 / kinematics[sptr].mf);
            currMassTons -= burnedFuel + detachedEngines * S.engine->mass + tankWeight;
            nEngines -= detachedEngines;
            sptr++;
        }
    }
}

double RocketConfig::remainingDeltaV(const std::vector<StageKinematics>& kinematics, float elapsed) const {
    float stageTime = elapsed;
    for (std::size_t s = 0; s < kinematics.size(); ++s) {
        if (stageTime < kinematics[s].burnTime) {
            float flowRate = (kinematics[s].m0 - kinematics[s].mf) / kinematics[s].burnTime;
            float currentMass = kinematics[s].m0 - flowRate * stageTime;
            float remFlow = flowRate * (kinematics[s].burnTime - stageTime);
            float dV = 0;
            if (remFlow > 0) {
                dV += kinematics[s].engine->enginePerf.vacuumISP * KspSystem::Kerbin.surfaceGravity
                    * std::log(currentMass / (currentMass - remFlow));
            }
            for (int s2 = s + 1; s2 < kinematics.size(); ++s2) {
                dV += kinematics[s2].vacuumDeltaV;
            }
            return dV;
        }
        stageTime -= kinematics[s].burnTime;
    }
    return 0;
}

