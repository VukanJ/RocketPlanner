#include "RocketSolver.h"
#include "helper.h"
#include "NelderMead.h"

#include <bit>
#include <cmath>
#include <algorithm>
#include <bitset>

static std::vector<double> softmaxFractions(const std::vector<double>& params) {
    int n = params.size() + 1;
    double maxVal = 0.0;  // anchor is always 0
    for (int i = 0; i < n - 1; ++i) {
        if (params[i] > maxVal) maxVal = params[i];
    }

    std::vector<double> exps(n);
    double sumExp = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        exps[i] = std::exp(params[i] - maxVal);
        sumExp += exps[i];
    }
    exps[n - 1] = std::exp(-maxVal);
    sumExp += exps[n - 1];

    std::vector<double> fractions(n);
    for (int i = 0; i < n; ++i) {
        fractions[i] = exps[i] / sumExp;
    }
    return fractions;
}

static std::vector<double> deltaVFromParams(const std::vector<double>& params, double target) {
    auto fractions = softmaxFractions(params);
    std::vector<double> deltaV(fractions.size());
    for (int i = 0; i < (int)fractions.size(); ++i) {
        deltaV[i] = fractions[i] * target;
    }
    return deltaV;
}

static double computeAsparagusFullMass(
    const std::vector<double>& fuelFractions,
    const PartProperty* engine,
    int engineMultiplicity,
    double targetDeltaV,
    double payloadMass,
    double minTWR,
    int baseSymmetry,
    int numStages,
    std::uint16_t engineConfig,
    double g0)
{
    // Compute the total mass of an asparagus stage needed to reach a targeted deltaV.
    // This is needed so that the coarse staging optimization remains valid even if a stage is 
    // subdivided into multiple asparagus substages. 
    // Returns the asparagus staged config
    
    // Compute the maximum fuel mass needed without asparagus
    double enginesMass = engine->getMass() * engineMultiplicity;
    double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * g0));
    if ((R - 1.0) / 8.0 >= 1.0) {
        return INFINITY;
    }
    double mEmpty = (payloadMass + enginesMass) / (1 - (R - 1.0) / 8.0);
    double mFMax = mEmpty * (R - 1.0);

    // Find fuel mass that achieves the target deltaV with the given asparagus configuration using bisection.
    double A = mFMax / 4.0;
    double B = mFMax;
    double mF = (A + B) / 2.0;
    for (int nIter = 0; nIter < 6; ++nIter) { // About 1% precision
        double mCurrent = mEmpty + mF;
        double productFullMasses = mCurrent;
        mCurrent -= fuelFractions[0] * mF;
        double productFinalMasses = mCurrent;

        for (int s = 1; s <= numStages; ++s) {
            double burnedFuel           = fuelFractions[s - 1] * mF;
            double detachedEngineWeight = (engineConfig >> (s - 1) & 0x1) * (baseSymmetry * engine->getMass());
            double tankWeight           = burnedFuel / 9.0;

            mCurrent -= tankWeight + detachedEngineWeight;
            productFullMasses *= mCurrent;
            mCurrent -= fuelFractions[s] * mF;
            productFinalMasses *= mCurrent;
        }

        double achievedDeltaV = engine->enginePerf.vacuumISP * g0 * std::log(productFullMasses / productFinalMasses);
        if (achievedDeltaV < targetDeltaV) {
            A = mF;
        } else {
            B = mF;
        }
        mF = (A + B) / 2.0;
    }

    double mFull = mEmpty + mF;
    double TWR = (engine->MaxThrustkN * engineMultiplicity) / (mFull * g0);
    if (TWR < minTWR) {
        return INFINITY;
    }
    return mFull;
}


RocketSolver::RocketSolver(const PartInfoList engines) : allEngines(engines) { }

void RocketSolver::solve(double targetDeltaV, double payloadMass, double minTWR, double g0) {
    RocketConfig bestConfig;
    bestConfig.totalMass = INFINITY;

    for (int nstage = 1; nstage < 8; ++nstage) {
        std::vector<double> deltaV;

        if (nstage == 1) {
            deltaV = {targetDeltaV};
        } else {
            auto objective = [&](const std::vector<double>& p) -> double {
                auto dv = deltaVFromParams(p, targetDeltaV);
                return buildRocket(dv, payloadMass, minTWR, g0).totalMass;
            };

            std::vector<double> x0(nstage - 1, 0.0);
            auto best = minimize(objective, x0);
            auto optDV = deltaVFromParams(best, targetDeltaV);
            auto optRocket = buildRocket(optDV, payloadMass, minTWR, g0);
            auto eqRocket = buildRocket(deltaVFromParams(x0, targetDeltaV), payloadMass, minTWR, g0);
            deltaV = (optRocket.totalMass <= eqRocket.totalMass) ? optDV : deltaVFromParams(x0, targetDeltaV);
        }

        auto rocket = buildRocket(deltaV, payloadMass, minTWR, g0);
        if (rocket.totalMass < bestConfig.totalMass) {
            bestConfig = rocket;
        }

        if (rocket.totalMass < INFINITY) {
            println("=========== ROCKET WITH STAGES:", nstage, ", Mass:", rocket.totalMass, "===========");
            for (int stage = 0; stage < nstage; ++stage) {
                const auto& info = rocket.stages[stage];
                int boosterCount = std::popcount((unsigned int)info.asparagus_config.hasEngine)
                                 * info.asparagus_config.baseSymmetry;
                println("  Stage", stage + 1, ": ", info.engine->title,
                        " x", info.engineMultiplicity,
                        boosterCount > 0 ? " + booster x" + std::to_string(boosterCount) : std::string{},
                        ", full=", info.fullMass, ", TWR=", info.TWR);
                if (info.asparagus_config.baseSymmetry > 0) {
                    println("    Asparagus config: base symmetry=", info.asparagus_config.baseSymmetry,
                            ", num substages=", info.asparagus_config.numAsparagusStages,
                            ", engine config=", std::bitset<16>(info.asparagus_config.hasEngine));
                    println("Fuel fractions");
                    int s = 0;
                    for (auto f : info.asparagus_config.fuelFractions) {
                        println("`\t\t\tsubStage", s++, ": ", f);
                    }
                }
            }
        } else {
            println("=========== ROCKET WITH STAGES:", nstage, " ===========");
            println("  No valid configuration found.");
        }
    }

    println("Best: ", bestConfig.stages.size(), " stages, total mass: ", bestConfig.totalMass, "t");
}

RocketSolver::StageInfo inline calcStageMass(const PartProperty* engine, 
                                             int engineMultiplicity, 
                                             double targetDeltaV, 
                                             double payloadMass, 
                                             double minTWR, 
                                             int asparagusBaseSymmetry,
                                             int asparagusNumStages,
                                             std::uint16_t asparagusEngineConfig,
                                             double g0) 
{
    RocketSolver::StageInfo info;

    if (asparagusBaseSymmetry > 0) {
        // Asparagus
        std::vector<double> fuelFractions(asparagusNumStages + 1);
        if (asparagusNumStages == 0) {
            fuelFractions[0] = 1.0;
        } else {
            auto objective = [&](const std::vector<double>& p) -> double {
                return computeAsparagusFullMass(softmaxFractions(p), engine, engineMultiplicity,
                                                targetDeltaV, payloadMass, minTWR,
                                                asparagusBaseSymmetry, asparagusNumStages,
                                                asparagusEngineConfig, g0);
            };
            std::vector<double> x0(asparagusNumStages, 0.0);
            NelderMeadParams nmParams;
            nmParams.bestValueTol = 1e-4;
            auto best = minimize(objective, x0, nmParams);
            auto optFractions = softmaxFractions(best);
            auto eqFractions = softmaxFractions(x0);
            double mOpt = computeAsparagusFullMass(optFractions, engine, engineMultiplicity,
                                                   targetDeltaV, payloadMass, minTWR,
                                                   asparagusBaseSymmetry, asparagusNumStages,
                                                   asparagusEngineConfig, g0);
            double mEq = computeAsparagusFullMass(eqFractions, engine, engineMultiplicity,
                                                  targetDeltaV, payloadMass, minTWR,
                                                  asparagusBaseSymmetry, asparagusNumStages,
                                                  asparagusEngineConfig, g0);
            fuelFractions = (mOpt <= mEq) ? optFractions : eqFractions;
        }

        double mFull = computeAsparagusFullMass(fuelFractions, engine, engineMultiplicity,
                                                targetDeltaV, payloadMass, minTWR,
                                                asparagusBaseSymmetry, asparagusNumStages,
                                                asparagusEngineConfig, g0);
        if (mFull == INFINITY) {
            info.fullMass = INFINITY;
            return info;
        }

        double enginesMass = engine->getMass() * engineMultiplicity;
        double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * g0));
        double mEmpty = (payloadMass + enginesMass) / (1 - (R - 1.0) / 8.0);
        int totalEngines = engineMultiplicity + std::popcount((unsigned int)asparagusEngineConfig) * asparagusBaseSymmetry;
        double TWR = (engine->MaxThrustkN * totalEngines) / (mFull * g0);

        info.asparagus_config.fuelFractions = fuelFractions;
        info.fullMass  = mFull;
        info.emptyMass = mEmpty;
        info.TWR       = TWR;
    }
    else {
        double enginesMass = engine->getMass() * engineMultiplicity;
        double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * Constants::g0_kerbin));
        if ((R - 1.0) / 8.0 >= 1.0) {
            // Does not converge. Dry mass too high, cannot be offset by more fuel.
            info.fullMass = INFINITY;
            return info;
        }
        
        double mEmpty = (payloadMass + enginesMass) / (1 - (R - 1.0) / 8.0);
        double mFull = mEmpty * R;
        double TWR = (engine->MaxThrustkN * engineMultiplicity) / (mFull * g0);
        if (TWR < minTWR) {
            info.fullMass = INFINITY;
            return info;
        }
        info.fullMass  = mFull;
        info.emptyMass = mEmpty;
        info.TWR       = TWR;
    }

    info.engine             = engine;
    info.engineMultiplicity = engineMultiplicity;
    info.asparagus_config.baseSymmetry       = asparagusBaseSymmetry;
    info.asparagus_config.numAsparagusStages = asparagusNumStages;
    info.asparagus_config.hasEngine          = asparagusEngineConfig;
    return info;
}


RocketSolver::StageInfo RocketSolver::solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0) {
    // Find best stage configuration for single stage rocket
    // Best = lowest wet mass rocket fulfilling the Δv and TWR requirements
    StageInfo bestStage;
    // Iterate over available engines
    for (const auto& engine : allEngines) {
        // Iterate over allowed engine multiplicities (number of engines in the stage)
        for (int mult = 1; mult <= 8; ++mult) {

            // Iterate over allowed asparagus configurations.
            // Base symmetry: 0 (no asparagus). 
            for (int baseSymmetry = 0; baseSymmetry <= MAX_ASPARAGUS_SYMMETRY; ++baseSymmetry) {
                if (baseSymmetry == 1) { continue; } // Asymmetric mass distribution
                // Iterate over number of asparagus substages. 0 Means only base symmetry pumping to middle.
                for (int asparagusStages = 0; asparagusStages <= MAX_ASPARAGUS_SUBSTAGES; ++asparagusStages) {
                    // Now iterate over all possible configurations of which asparagus boosters have engines.
                    // If there are three substages, then there are 000 to 111 (0 to 7) configurations
                    // 0 means, the stage is a simple drop tank. 000 means, all engines are on the main stage
                    const std::uint16_t maxConfig = (1 << asparagusStages) - 1;
                    for (std::uint16_t econfig = 0x0; econfig <= maxConfig; ++econfig) {

                        const auto stage = calcStageMass(engine, mult, targetDeltaV, payloadMass, minTWR, baseSymmetry, asparagusStages, econfig, g0);
                        if (stage.fullMass < bestStage.fullMass) {
                            bestStage = stage;
                        }

                    }
                }
            }
        }
    }
    return bestStage;
}

RocketSolver::RocketConfig RocketSolver::buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0) {
    // deltaVPerStage = [deltaV_stage1, deltaV_stage2, ..., deltaV_stage_last]
    const int nStages = deltaVPerStage.size();
    RocketConfig config;
    config.stages.resize(nStages);

    double stagePayload = payloadMass;
    for (int stage = nStages - 1; stage >= 0; --stage) {
        config.stages[stage] = solveSingleStage(deltaVPerStage[stage], stagePayload, minTWR, g0);
        stagePayload += config.stages[stage].fullMass; // The payload for the next stage includes the mass of the current stage
    }
    config.totalMass = stagePayload;
    return config;
}
