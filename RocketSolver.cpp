#include "RocketSolver.h"
#include "helper.h"
#include "NelderMead.h"

#include <cmath>

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

RocketSolver::StageInfo RocketSolver::solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0) {
    // Find best stage configuration for single stage rocket
    StageInfo bestStage;
    for (const auto& engine : allEngines) {
        for (int mult = 1; mult <= 8; ++mult) {
            double engineMass = engine->getMass(1.0) * mult;
            double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * Constants::g0_kerbin));
            if ((R - 1.0) / 8.0 >= 1.0) {
                continue; // Does not converge
            }
    
            double mEmpty = (payloadMass + engineMass) / (1 - (R - 1.0) / 8.0);
            double mFull = mEmpty * R;
            double TWR = (engine->MaxThrustkN * mult) / (mFull * g0);
            if (TWR < minTWR) {
                continue; // Not enough thrust
            }
            if (mFull < bestStage.fullMass) {
                bestStage.fullMass           = mFull;
                bestStage.emptyMass          = mEmpty;
                bestStage.engine             = engine;
                bestStage.engineMultiplicity = mult;
                bestStage.TWR                = TWR;
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
