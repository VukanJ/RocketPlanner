#include "RocketSolver.h"
#include "helper.h"
#include "NelderMead.h"

#include <cmath>
#include <algorithm>

static std::vector<double> deltaVFromParams(const std::vector<double>& params, double target) {
    // Maps n-1 unconstrained variables (params ∈ ℝ^(n-1)) to n Δv values summing to target.
    // The optimizer needs unconstrained variables (any ℝ), but Δv values must be positive
    // and sum to target. Softmax with a fixed zero anchor handles both:
    //   - exp(x) > 0 guarantees positive Δv
    //   - softmax fractions sum to 1, then scaled by target
    // params=[0,0,...] gives equal split (all stages get target/n).
    int n = params.size() + 1;
    std::vector<double> exps(n);
    double sumExp = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        exps[i] = std::exp(params[i]);
        sumExp += exps[i];
    }
    exps[n - 1] = 1.0;
    sumExp += 1.0;

    std::vector<double> deltaV(n);
    for (int i = 0; i < n; ++i) {
        deltaV[i] = (exps[i] / sumExp) * target;
    }
    return deltaV;
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
                println("  Stage", stage + 1, ": ", info.engine->title, " x", info.engineMultiplicity,
                        ", full=", info.fullMass, ", TWR=", info.TWR);
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
        std::vector<double> fuelFractions(asparagusNumStages + 1, 0.5 / (asparagusNumStages + 1)); // Equal fuel distribution among base and asparagus stages
        fuelFractions[0] = 0.5; // Base stage gets half the fuel as a default
        std::array<double, MAX_ASPARAGUS_SUBSTAGES + 1> stageMassesFull;
        std::array<double, MAX_ASPARAGUS_SUBSTAGES + 1> stageMassesEmpty;

        // Find total needed fuel to satisfy Δv requirement with the given
        // asparagus configuration by bisection. The total fuel mass should
        // surely be in the interval [mFmax/4,mFmax], where mFmax is the fuel
        // mass needed without asparagus. Asparagus is more efficient in
        // general, but probably not more than 4x efficient.

        // Calculate mFmax upper bound
        double enginesMass = engine->getMass() * engineMultiplicity;
        const double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * g0));
        const double mEmpty = (payloadMass +  enginesMass) / (1 - (R - 1.0) / 8.0);
        const double mFMax = mEmpty * (R - 1.0);


        // Run bisection for 20 iterations
        // Assume all same engines, so isp is the same for all stages.
        double A = mFMax / 4.0;
        double B = mFMax;
        double mF = (A + B) / 2.0;
        for (int nIter = 0; nIter < 20; ++nIter) {
            // Calculate stage masses just before each stage begins its burn and just right after it has burned the outermost fuel
            stageMassesFull[0] = mEmpty + mF; // Start with all stages full
            stageMassesEmpty[0] = stageMassesFull[0] - fuelFractions[0] * mF; // Subtract fuel burned in first asparagus stage
            double productFullMasses = stageMassesFull[0];
            double productFinalMasses = stageMassesEmpty[0];
            for (int s = 1; s <= asparagusNumStages; ++s) {
                // Subtract burnt fuel and tank and potentiall engine weight of previous stage
                double burnedFuel           = fuelFractions[s - 1] * mF;
                double detachedEngineWeight = (asparagusEngineConfig >> (s - 1) & 0x1) * (asparagusBaseSymmetry * engine->getMass() * engineMultiplicity);
                double tankWeight           = burnedFuel / 9.0;  // 1:9 tank mass ratio assumption

                stageMassesFull[s]  = stageMassesFull[s-1] - burnedFuel - detachedEngineWeight - tankWeight;
                stageMassesEmpty[s] = stageMassesFull[s] - fuelFractions[s] * mF; // Subtract fuel burned in this stage
                productFullMasses  *= stageMassesFull[s];
                productFinalMasses *= stageMassesEmpty[s];
            }

            double achievedDeltaV = engine->enginePerf.vacuumISP * g0 * std::log(productFullMasses / productFinalMasses);
            if (achievedDeltaV < targetDeltaV) {
                A = mF; // Need more fuel
            } else {
                B = mF; // Can do with less fuel
            }
            mF = (A + B) / 2.0;
        }
        double mFull = mEmpty + mF;
        double TWR = (engine->MaxThrustkN * engineMultiplicity) / (mFull * g0);
        if (TWR < minTWR) { 
            info.fullMass = INFINITY;
            return info;
        }
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
