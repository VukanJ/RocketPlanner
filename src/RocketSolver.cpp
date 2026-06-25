#include "RocketSolver.h"
#include "helper.h"
#include "NelderMead.h"
#include "kspConstants.h"

#include <bit>
#include <cmath>
#include <bitset>

#define CSV_DUMP
#ifdef CSV_DUMP
#include <fstream>
#endif

static void softmaxFractions(const std::vector<double>& params, std::vector<double>& result) {
    int n = params.size() + 1;
    result.resize(n);

    double maxVal = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        if (params[i] > maxVal) maxVal = params[i];
    }

    double sumExp = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        result[i] = std::exp(params[i] - maxVal);
        sumExp += result[i];
    }
    result[n - 1] = std::exp(-maxVal);
    sumExp += result[n - 1];

    double invSum = 1.0 / sumExp;
    for (int i = 0; i < n; ++i) {
        result[i] *= invSum;
    }
}

static void deltaVFromParams(const std::vector<double>& params, double target, std::vector<double>& result) {
    softmaxFractions(params, result);
    for (int i = 0; i < (int)result.size(); ++i) {
        result[i] *= target;
    }
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
    double g0,
    double atmPressure)
{
    // Compute the total mass of an asparagus stage needed to reach a targeted deltaV.
    // This is needed so that the coarse staging optimization remains valid even if a stage is 
    // subdivided into multiple asparagus substages. Returns the asparagus staged config

    double isp = (atmPressure == 0.0) ? engine->enginePerf.vacuumISP : engine->enginePerf.getISP(atmPressure);

    // Dry mass = payload + ALL engines (core + boosters), no tanks.
    double engineMass = engine->getMass();
    double coreEnginesMass = engineMass * engineMultiplicity;
    double boosterEnginesMass = (double)std::popcount((unsigned)engineConfig)
                              * baseSymmetry * engine->getMass();
    double mEmpty = payloadMass + coreEnginesMass + boosterEnginesMass;

    double R = std::exp(targetDeltaV / (isp * g0));
    double A = 0.0;
    double B = mEmpty * R * 5.0;
    double mF;

    for (int nIter = 0; nIter < 7; ++nIter) {
        mF = (A + B) / 2.0;
        double mCurrent = mEmpty + mF + mF / 9.0;
        double productFullMasses = mCurrent;
        mCurrent -= fuelFractions[0] * mF;
        double productFinalMasses = mCurrent;

        bool feasible = true;
        for (int s = 1; s <= numStages; ++s) {
            double burnedFuel           = fuelFractions[s - 1] * mF;
            double detachedEngineWeight = (engineConfig >> (s - 1) & 0x1) * (baseSymmetry * engineMass);
            double tankWeight           = burnedFuel / 9.0;

            double dropMass = tankWeight + detachedEngineWeight;
            if (dropMass >= mCurrent) { feasible = false; break; }
            mCurrent -= dropMass;

            // Check if remaining engines have enough thrust to maintain TWR
            int remainingEngines = engineMultiplicity
                                 + std::popcount((unsigned)(engineConfig >> s)) * baseSymmetry;
            if (engine->MaxThrustkN * remainingEngines / (mCurrent * g0) < minTWR) {
                feasible = false; break;
            }

            productFullMasses *= mCurrent;

            double nextFuel = fuelFractions[s] * mF;
            if (nextFuel >= mCurrent) { feasible = false; break; }
            mCurrent -= nextFuel;
            productFinalMasses *= mCurrent;
        }

        if (!feasible || productFullMasses <= 0.0 || productFinalMasses <= 0.0) {
            A = mF;
            continue;
        }

        double achievedDeltaV = isp * g0
                              * std::log(productFullMasses / productFinalMasses);
        if (achievedDeltaV < targetDeltaV) {
            A = mF;
        } else {
            B = mF;
        }
    }

    mF = (A + B) / 2.0;

    // Re-evaluate dv at final mF to reject cases where B itself was insufficient.
    double mFull = mEmpty + mF + mF / 9.0;
    double mCurrent = mFull;
    double productFullMasses = mCurrent;
    mCurrent -= fuelFractions[0] * mF;
    double productFinalMasses = mCurrent;
    for (int s = 1; s <= numStages; ++s) {
        double burnedFuel           = fuelFractions[s - 1] * mF;
        double detachedEngineWeight = (engineConfig >> (s - 1) & 0x1) * (baseSymmetry * engineMass);
        double dropMass = burnedFuel / 9.0 + detachedEngineWeight;
        if (dropMass >= mCurrent) return INFINITY;
        mCurrent -= dropMass;

        int remainingEngines = engineMultiplicity
                             + std::popcount((unsigned)(engineConfig >> s)) * baseSymmetry;
        if (engine->MaxThrustkN * remainingEngines / (mCurrent * g0) < minTWR) return INFINITY;

        productFullMasses *= mCurrent;
        double nextFuel = fuelFractions[s] * mF;
        if (nextFuel >= mCurrent) return INFINITY;
        mCurrent -= nextFuel;
        productFinalMasses *= mCurrent;
    }
    if (productFullMasses <= 0.0 || productFinalMasses <= 0.0) return INFINITY;
    double achievedDeltaV = isp * g0
                          * std::log(productFullMasses / productFinalMasses);
    if (achievedDeltaV < targetDeltaV) return INFINITY;

    int totalEngines = engineMultiplicity + std::popcount((unsigned int)engineConfig) * baseSymmetry;
    double TWR = (engine->MaxThrustkN * totalEngines) / (mFull * g0);
    if (TWR < minTWR) return INFINITY;
    return mFull;
}


RocketSolver::RocketSolver(const PartInfoList engines) : allEngines(engines) { }

void RocketSolver::solve(double targetDeltaV, double payloadMass, double minTWR, double g0, double seaLevelAtm) {
    RocketConfig bestConfig;
    bestConfig.totalMass = INFINITY;

    for (int nstage = 1; nstage < 8; ++nstage) {
        std::vector<double> deltaV;

        if (nstage == 1) {
            deltaV = {targetDeltaV};
        } else {
            std::vector<double> dvBuf, optDV, eqDV;
            auto objective = [&](const std::vector<double>& p) -> double {
                deltaVFromParams(p, targetDeltaV, dvBuf);
                return buildRocket(dvBuf, payloadMass, minTWR, g0, seaLevelAtm).totalMass;
            };

            std::vector<double> x0(nstage - 1, 0.0);
            auto best = minimize(objective, x0, NelderMeadParams{.bestValueTol = 1e-3});
            deltaVFromParams(best, targetDeltaV, optDV);
            auto optRocket = buildRocket(optDV, payloadMass, minTWR, g0, seaLevelAtm);
            deltaVFromParams(x0, targetDeltaV, eqDV);
            auto eqRocket = buildRocket(eqDV, payloadMass, minTWR, g0, seaLevelAtm);
            deltaV = (optRocket.totalMass <= eqRocket.totalMass) ? optDV : eqDV;
        }

        auto rocket = buildRocket(deltaV, payloadMass, minTWR, g0, seaLevelAtm);
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
                                             double g0,
                                             double atmPressure) 
{
    static int callCount = 0;
    callCount++;
    //if (callCount % 10000 == 0) {
    //    println("calcStageMass calls: ", callCount);
    //}
    RocketSolver::StageInfo info;

    if (asparagusBaseSymmetry > 0) {
        // Asparagus
        std::vector<double> fuelFractions(asparagusNumStages + 1);
        if (asparagusNumStages == 0) {
            fuelFractions[0] = 1.0;
        } else {
            std::vector<double> fracBuf, optFractions, eqFractions;
            auto objective = [&](const std::vector<double>& p) -> double {
                softmaxFractions(p, fracBuf);
                return computeAsparagusFullMass(fracBuf, engine, engineMultiplicity,
                                                targetDeltaV, payloadMass, minTWR,
                                                asparagusBaseSymmetry, asparagusNumStages,
                                                asparagusEngineConfig, g0, atmPressure);
            };
            std::vector<double> x0(asparagusNumStages, 0.0);

            NelderMeadParams nmParams;
            nmParams.bestValueTol = 1e-2;
            nmParams.initialStep = 1.5;
            auto best = minimize(objective, x0, nmParams);
            softmaxFractions(best, optFractions);
            softmaxFractions(x0, eqFractions);
            double mOpt = computeAsparagusFullMass(optFractions, engine, engineMultiplicity,
                                                   targetDeltaV, payloadMass, minTWR,
                                                   asparagusBaseSymmetry, asparagusNumStages,
                                                   asparagusEngineConfig, g0, atmPressure);
            double mEq = computeAsparagusFullMass(eqFractions, engine, engineMultiplicity,
                                                  targetDeltaV, payloadMass, minTWR,
                                                  asparagusBaseSymmetry, asparagusNumStages,
                                                  asparagusEngineConfig, g0, atmPressure);
            fuelFractions = (mOpt <= mEq) ? optFractions : eqFractions;
        }

        double mFull = computeAsparagusFullMass(fuelFractions, engine, engineMultiplicity,
                                                targetDeltaV, payloadMass, minTWR,
                                                asparagusBaseSymmetry, asparagusNumStages,
                                                asparagusEngineConfig, g0, atmPressure);
        if (mFull == INFINITY) {
            info.fullMass = INFINITY;
            return info;
        }

        double coreEnginesMass = engine->getMass() * engineMultiplicity;
        double boosterEnginesMass = (double)std::popcount((unsigned int)asparagusEngineConfig)
                                  * asparagusBaseSymmetry * engine->getMass();
        double massNoFuelNoTanks = payloadMass + coreEnginesMass + boosterEnginesMass;
        double totalTankMass = (mFull - massNoFuelNoTanks) / 10.0;
        double mEmpty = massNoFuelNoTanks + totalTankMass;
        int totalEngines = engineMultiplicity + std::popcount((unsigned int)asparagusEngineConfig) * asparagusBaseSymmetry;
        double TWR = (engine->MaxThrustkN * totalEngines) / (mFull * g0);

        info.asparagus_config.fuelFractions = fuelFractions;
        info.fullMass  = mFull;
        info.emptyMass = mEmpty;
        info.TWR       = TWR;
    }
    else {
        info.asparagus_config.fuelFractions = {1.0};
        double isp = (atmPressure == 0.0) ? engine->enginePerf.vacuumISP : engine->enginePerf.getISP(atmPressure);
        double enginesMass = engine->getMass() * engineMultiplicity;
        double R = std::exp(targetDeltaV / (isp * KspSystem::Kerbin.surfaceGravity));
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


RocketSolver::StageInfo RocketSolver::solveSingleStage(double targetDeltaV, double payloadMass, double minTWR, double g0, double atmPressure) {
    // Find best stage configuration for single stage rocket
    // Best = lowest wet mass rocket fulfilling the Δv and TWR requirements
    StageInfo bestStage;
    // Iterate over available engines
    for (const auto& engine : allEngines) {
        for (int mult = 1; mult <= 4; ++mult) {
            // Iterate over allowed asparagus configurations.
            // Base symmetry: 0 (no asparagus). 
            for (int baseSymmetry = 0; baseSymmetry <= MAX_ASPARAGUS_SYMMETRY; ++baseSymmetry) {
                if (baseSymmetry == 1) { continue; } // Asymmetric mass distribution
                // Iterate over number of asparagus substages. 0 Means only base symmetry pumping to middle.
                for (int asparagusStages = 0; asparagusStages <= MAX_ASPARAGUS_SUBSTAGES; ++asparagusStages) {
                    if (baseSymmetry == 0 && asparagusStages > 0) continue; // No asparagus without base symmetry
                    // Now iterate over all possible configurations of which asparagus boosters have engines.
                    // If there are three substages, then there are 000 to 111 (0 to 7) configurations
                    // 0 means, the stage is a simple drop tank. 000 means, all engines are on the main stage
                    const std::uint16_t maxConfig = (1 << asparagusStages) - 1;
                    for (std::uint16_t econfig = 0x0; econfig <= maxConfig; ++econfig) {

                        const auto stage = calcStageMass(engine, mult, targetDeltaV, payloadMass, minTWR, baseSymmetry, asparagusStages, econfig, g0, atmPressure);
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

void integrate_ascent(float liftoffTWR, Body body) {
    // Calculate the (rough) ascent profile of a rocket of a certain TWR. 
    // This is quite crude, but better than nothing.
    auto AltPressure_atm = [&body](double alt_km) -> double {
        return alt_km > body.atmHeight_km ? 0 : body.seaLevel_atm * std::exp(-alt_km / body.atm_falloff_km);
    };
    auto getGravity = [&body](double alt_km) -> double {
        return body.surfaceGravity * std::pow(body.radius_km / (body.radius_km + alt_km), 2);
    };

    auto A = [&body](double alt_km) {
        // Assume that rocket starts with some projected airstream area at liftoff which exponentiall decays 
        // with each stage. Assume that area decays with the same rate as pressure
        double Ainitial = Constants::mk1_area_m2 * 8 + Constants::mk2_area_m2;
        double Afinal = Constants::mk2_area_m2;
        double alpha = body.atmHeight_km / log(Ainitial / Afinal);
        return Ainitial * std::exp(-alt_km / alpha);
    };

    auto rocketMass = [&body](double alt_km) {
      // Assume that rocket starts with some mass  at liftoff which exponentiall
      // decays with each stage. The reason is that fuel consumption becomes
      // slower as more engines are dropped.
      double Minitial = 50000;
      double Mfinal = 5000;
      double alpha = body.atmHeight_km / log(Minitial / Mfinal);
      return std::max(Mfinal, Minitial * std::exp(-alt_km / alpha));
    };

    constexpr double Cd = 0.2;  // Most parts have a minimum drag coefficient of around 0.2, sometimes less.

    auto DragAccel = [&](double alt_km, double v) {
        // Newtonian drag: Fd = 0.5 * rho * v^2 * A * Cd
        double rho = AltPressure_atm(alt_km) * body.sea_level_density_kgpm3 / body.seaLevel_atm;
        return 0.5 * rho * v * v * A(alt_km) * Cd;
    };

    double dt = 1;
    double alt = 0.0;
    double v = 0.0;

    for (int i = 0; i < 300; ++i) {
        double drag_accel = DragAccel(alt*0.001, v) / rocketMass(alt*0.001);

        v += (liftoffTWR*body.surfaceGravity - getGravity(alt*0.001) - drag_accel) * dt;
        alt += v * dt;
        println("Time:", i, "s, Altitude:", alt*0.001, "km, Velocity:", v, "m/s, Gravity:", getGravity(alt*0.001), "m/s^2, Atm Pressure:", AltPressure_atm(alt*0.001), "atm");
    }

    //exit(0);

}

int RocketSolver::RocketConfig::totalStages() const {
    int tot = 0;
    for (const auto& stage : stages) {
        tot += 1 + stage.asparagus_config.numAsparagusStages;
    }
    return tot;
}

void RocketSolver::RocketConfig::recomputeMasses(const std::vector<double>& fuelMass) {
    double belowMass = 0.0;
    for (int i = (int)stages.size() - 1; i >= 0; --i) {
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
        if (stage.asparagus_config.baseSymmetry == 0)
            stage.asparagus_config.fuelFractions = {1.0};
        else if ((int)stage.asparagus_config.fuelFractions.size() != stage.asparagus_config.numAsparagusStages + 1)
            stage.asparagus_config.fuelFractions.assign(stage.asparagus_config.numAsparagusStages + 1, 1.0 / (stage.asparagus_config.numAsparagusStages + 1));
        belowMass = stage.fullMass;
    }
}

void RocketSolver::RocketConfig::calcStageKinematics(std::vector<StageKinematics>& kinematics) const {
    const int totStages = totalStages();
    kinematics.resize(totStages);

    int sptr = 0;
    for (int stage = 0; stage < stages.size(); ++stage) {
        const auto& stageInfo = stages[stage];
        const double totalFuel_tons = stageInfo.fullMass - stageInfo.emptyMass;
        double currMass_tons = stageInfo.fullMass;
        int nEngines = stageInfo.engineMultiplicity + std::popcount(stageInfo.asparagus_config.hasEngine) * stageInfo.asparagus_config.baseSymmetry;
        int asparagus = stageInfo.asparagus_config.numAsparagusStages;
        int boosters = stageInfo.asparagus_config.baseSymmetry;
        for (int sub = 0; sub < asparagus + 1; ++sub) {
            kinematics[sptr].engine = stageInfo.engine;
            kinematics[sptr].nEngines = nEngines;
            int boosters_attached = (asparagus == 0) ? 0 : (sub < asparagus ? boosters - sub : 0);
            kinematics[sptr].area_m2 = Constants::mk2_area_m2 + boosters_attached * Constants::mk1_area_m2;
            double burnedFuel = stageInfo.asparagus_config.fuelFractions[sub] * totalFuel_tons;
            int detachedEngines = sub == asparagus ? stageInfo.engineMultiplicity
                                                    : ((stageInfo.asparagus_config.hasEngine >> sub) & 0x1) * boosters;
            double tankWeight = burnedFuel / 9.0;
            kinematics[sptr].m0 = currMass_tons;
            kinematics[sptr].mf = currMass_tons - burnedFuel;
            kinematics[sptr].burnTime = (burnedFuel / stageInfo.engine->usedFuelDensity()) / (stageInfo.engine->enginePerf.fuelConsumptionRate_UPS * nEngines);
            currMass_tons -= burnedFuel + detachedEngines*stageInfo.engine->getMass() + tankWeight;
            nEngines -= detachedEngines;
            sptr++;
        }
    }
}

void simulate_flight(Body body, const RocketSolver::RocketConfig& rocket) {
    struct vec { float x = 0; float y = 0; };
    vec pos {0, body.radius_km};
    vec vel {2.0f * M_PI * body.radius_km * 1000.0f / body.rotPeriod_s, 0};
    vec dir {0, 1};

    std::vector<StageKinematics> kinematics;
    rocket.calcStageKinematics(kinematics);

    auto getApoapsisHeight = [&pos, &vel, &body]() -> float {
        double vx = vel.x / 1000.0;                    // m/s → km/s
        double vy = vel.y / 1000.0;
        double v2 = vx*vx + vy*vy;                     // km²/s²
        double r  = std::sqrt(pos.x*pos.x + pos.y*pos.y); // km
        double eps = 0.5*v2 - body.GM() / r;           // km²/s²
        if (eps >= 0) return INFINITY;
        double a   = -body.GM() / (2*eps);             // km
        double h   = pos.x * vy - pos.y * vx;          // km²/s
        double e   = std::sqrt(1 + 2*eps*h*h / (body.GM()*body.GM()));
        return a * (1 + e) - body.radius_km;           // apoapsis height in km
    };

    float dt = 1;

#ifdef CSV_DUMP
    std::ofstream csv("flight_log.csv");
    csv << "t,alt_km,vel_m_s,vx_m_s,vy_m_s,posx_km,posy_km,thrust_kN,mass_t,pressure_atm,dir_angle_deg,apoapsis_km,stage,A_m2,drag_N\n";
#endif

    const int nStage = kinematics.size();
    int stage = 0;
    StageKinematics currentStage = kinematics[0];
    float elapsed = 0;
    float stageTime = 0;
    float mass = currentStage.m0;
    float flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;
    for (int i = 0; i < 500; ++i) {
        if (stageTime > currentStage.burnTime) {
            if (stage == nStage - 1) {
                println("BURNED OUT");
                break;
            }
            println("STAGE");
            stage++;
            currentStage = kinematics[stage];
            stageTime = 0;
            mass = currentStage.m0;
            flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;
        }

        float altitude = std::sqrt(pos.x*pos.x + pos.y*pos.y) - body.radius_km;
        float pressure = body.getPressureAtAltitude_km(altitude);

        // Gravity vector: always points toward planet center
        float r = altitude + body.radius_km;
        float g_mag = body.surfaceGravity * std::pow(body.radius_km / r, 2);
        vec grav = {-g_mag * pos.x / r, -g_mag * pos.y / r};

        // Gravity turn: steer from vertical toward horizontal as pressure drops
        {
            float r = std::sqrt(pos.x*pos.x + pos.y*pos.y);
            vec up  {pos.x / r, pos.y / r};
            vec east{up.y, -up.x};
            float angle_rad;
            constexpr float startATM = 1.0f;
            if (pressure > startATM) {
                angle_rad = 0.0f;
            } else if (pressure <= 0.0f) {
                angle_rad = 85.0f * M_PI / 180.0f;
            } else {
                float t = pressure / startATM;
                angle_rad = (1.0f - t) * 85.0f * M_PI / 180.0f;
            }
            dir.x = cos(angle_rad) * up.x + sin(angle_rad) * east.x;
            dir.y = cos(angle_rad) * up.y + sin(angle_rad) * east.y;
        }

        float isp = currentStage.engine->enginePerf.getISP(pressure);
        float ispVac = currentStage.engine->enginePerf.vacuumISP;
        float thrust = (isp / ispVac) * currentStage.engine->MaxThrustkN * currentStage.nEngines;

        // Drag: cross-sectional area from current stage's exposed hardware
        double A = currentStage.area_m2;
        constexpr double Cd = 0.2;
        double rho = (double)pressure * body.sea_level_density_kgpm3 / body.seaLevel_atm;
        double speed_d = std::sqrt((double)vel.x * vel.x + (double)vel.y * vel.y);
        double drag_mag = 0.5 * rho * speed_d * speed_d * A * Cd;
        float drag_N = (float)drag_mag;
        vec drag_accel{0, 0};
        if (speed_d > 1e-6) {
            float ax = (float)(-drag_mag / (mass * 1000.0) * vel.x / speed_d);
            float ay = (float)(-drag_mag / (mass * 1000.0) * vel.y / speed_d);
            drag_accel = {ax, ay};
        }

        float accel_x = thrust * dir.x / mass + drag_accel.x + grav.x;
        float accel_y = thrust * dir.y / mass + drag_accel.y + grav.y;
        vel.x += accel_x * dt;
        vel.y += accel_y * dt;
        pos.x += vel.x * dt / 1000.0;
        pos.y += vel.y * dt / 1000.0;

        mass -= flowRate * dt;

        float apo = getApoapsisHeight();
        float speed = std::sqrt(vel.x*vel.x + vel.y*vel.y);
        println("t=", elapsed, "APO: ", apo, "km, Altitude: ", altitude, "km, Velocity: ", speed, "m/s, Mass: ", mass, "t");

#ifdef CSV_DUMP
        double dir_angle_deg = std::atan2(dir.x, dir.y) * 180.0 / M_PI;
        csv << elapsed << ","
            << altitude << ","
            << speed << ","
            << vel.x << ","
            << vel.y << ","
            << pos.x << ","
            << pos.y << ","
            << thrust << ","
            << mass << ","
            << pressure << ","
            << dir_angle_deg << ","
            << apo << ","
            << stage << ","
            << A << ","
            << drag_N << "\n";
#endif
        
        stageTime += dt;
        elapsed += dt;
    }
#ifdef CSV_DUMP
    exit(0);
#endif
}

RocketSolver::RocketConfig RocketSolver::buildRocket(const std::vector<double>& deltaVPerStage, double payloadMass, double minTWR, double g0, double seaLevelAtm) {
    // deltaVPerStage = [deltaV_stage1, deltaV_stage2, ..., deltaV_stage_last]
    const int nStages = deltaVPerStage.size();
    RocketConfig config;
    config.stages.resize(nStages);

    //integrate_ascent(1.7, KspSystem::Eve);

    double stagePayload = payloadMass;
    for (int stage = nStages - 1; stage >= 0; --stage) {
        double atmPressure = (stage == 0) ? seaLevelAtm : 0.0;
        config.stages[stage] = solveSingleStage(deltaVPerStage[stage], stagePayload, minTWR, g0, atmPressure);
        stagePayload = config.stages[stage].fullMass; // The next stage carries this stage as payload
    }
    config.totalMass = stagePayload;

    // Debug
    if (config.totalMass < INFINITY) {
        simulate_flight(KspSystem::Eve, config);
    }
    //

    return config;
}
