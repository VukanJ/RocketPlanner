#include "RocketSolver.h"
#include "helper.h"

#include <cmath>

RocketSolver::RocketSolver(const PartInfoList engines) : allEngines(engines) { }

void RocketSolver::solve(double targetDeltaV, double payloadMass, double minTWR, double g0) {
    for (int nstage = 1; nstage < 5; ++nstage) {
        std::vector<double> deltaVPerStage(nstage, targetDeltaV / nstage); // Initial guess: equal deltaV per stage
        auto rocket = buildRocket(deltaVPerStage, payloadMass, minTWR, g0);
        println("=========== ROCKET WITH STAGES:", nstage, "===========");
        if (rocket.totalMass < INFINITY) {
            println("Total Mass:", rocket.totalMass);
            for (int stage = 0; stage < nstage; ++stage) {
                const auto& info = rocket.stages[stage];
                println(" Stage", stage + 1, ":");
                println("  Engine:", info.engine->title, "x", info.engineMultiplicity);
                println("  Full Mass:", info.fullMass);
                println("  Empty Mass:", info.emptyMass);
                println("  TWR:", info.TWR);
            }
        }
        else {
            println("No valid configuration found.");
        }
    }
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
