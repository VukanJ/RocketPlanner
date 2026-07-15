#include <gtest/gtest.h>
#include <cmath>
#include <numbers>

#include "DeltaV.h"
#include "RocketSolver.h"
#include "imgui.h"
#include "parts.h"
#include "kspConstants.h"
#include "utils.h"
#include "Orbit.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Create a minimal LF engine for use in kinematic tests.
static PartProperty makeTestEngine(double engineMass, double thrustkN, double vacuumISP) {
    EngineISPInfo isp;
    isp.isp = {{0.0, vacuumISP}};
    return PartProperty(
        PartType::LFEngine, "Test Engine",
        engineMass,    // mass in tons
        1, 1,          // att_top, att_bot
        1.0f,          // length
        0,             // maxCrew
        {},            // no resources
        thrustkN,
        isp
    );
}

// Build a non-asparagus stage with consistent masses.
// emptyMass = payload + nEngine*engineMass + fuelMass/9  (tanks included)
// fullMass  = emptyMass + fuelMass
// fuelFractions = {1.0}
static StageInfo makeNonAsparagusStage(
    double payloadMass, const PartProperty& engine,
    int engineMultiplicity, double fuelMass)
{
    StageInfo s;
    double engineMass  = engine.getMass();
    double tankMass    = fuelMass / 9.0;
    s.emptyMass        = payloadMass + engineMultiplicity * engineMass + tankMass;
    s.fullMass         = s.emptyMass + fuelMass;
    s.engine           = &engine;
    s.engineMultiplicity = engineMultiplicity;
    s.asparagus_config.baseSymmetry       = 0;
    s.asparagus_config.numAsparagusStages = 0;
    s.asparagus_config.hasEngine          = 0;
    s.asparagus_config.fuelFractions      = {1.0};
    s.TWR = 0;
    return s;
}

// Build an asparagus stage with consistent masses (after the emptyMass fix).
// tankMass = fuelMass / 9
// massNoFuelNoTanks = payload + coreEngineMass + boosterEngineMass
// emptyMass = massNoFuelNoTanks + tankMass
// fullMass  = massNoFuelNoTanks + tankMass + fuelMass
static StageInfo makeAsparagusStage(
    double payloadMass, const PartProperty& engine,
    int engineMultiplicity, int baseSymmetry,
    int numAsparagusStages, std::uint16_t hasEngine,
    const std::vector<double>& fuelFractions)
{
    StageInfo s;
    double engineMass  = engine.getMass();
    double fuelMass    = 0;
    for (double f : fuelFractions) { fuelMass += f; }
    double tankMass    = fuelMass / 9.0;
    double boosterMass = std::popcount((unsigned)hasEngine) * baseSymmetry * engineMass;
    double coreMass    = engineMultiplicity * engineMass;
    double noFuelMass  = payloadMass + coreMass + boosterMass;

    s.fullMass         = noFuelMass + fuelMass + tankMass;
    // emptyMass includes tanks (consistent with non-asparagus convention)
    double totalTank   = (s.fullMass - noFuelMass) / 10.0;
    s.emptyMass        = noFuelMass + totalTank;
    s.engine           = &engine;
    s.engineMultiplicity = engineMultiplicity;
    s.asparagus_config.baseSymmetry       = baseSymmetry;
    s.asparagus_config.numAsparagusStages = numAsparagusStages;
    s.asparagus_config.hasEngine          = hasEngine;
    s.asparagus_config.fuelFractions      = fuelFractions;
    s.TWR = 0;
    return s;
}

// ---------------------------------------------------------------------------
// totalStages
// ---------------------------------------------------------------------------

TEST(TotalStagesTest, EmptyConfig) {
    RocketConfig cfg;
    EXPECT_EQ(cfg.totalStages(), 0);
}

TEST(TotalStagesTest, SingleNoAsparagus) {
    RocketConfig cfg;
    cfg.stages.resize(1);
    cfg.stages[0].asparagus_config.numAsparagusStages = 0;
    EXPECT_EQ(cfg.totalStages(), 1);
}

TEST(TotalStagesTest, SingleWithAsparagus) {
    RocketConfig cfg;
    cfg.stages.resize(1);
    cfg.stages[0].asparagus_config.numAsparagusStages = 2;
    EXPECT_EQ(cfg.totalStages(), 3);
}

TEST(TotalStagesTest, MixedStages) {
    RocketConfig cfg;
    cfg.stages.resize(3);
    cfg.stages[0].asparagus_config.numAsparagusStages = 1; // 2 substages
    cfg.stages[1].asparagus_config.numAsparagusStages = 0; // 1 substage
    cfg.stages[2].asparagus_config.numAsparagusStages = 3; // 4 substages
    EXPECT_EQ(cfg.totalStages(), 2 + 1 + 4);
}

// ---------------------------------------------------------------------------
// calcStageKinematics – mass chain invariants
// ---------------------------------------------------------------------------

class KinematicsTest : public ::testing::Test {
protected:
    // Engine properties used across tests
    static constexpr double kEngineMass   = 0.5;   // tons
    static constexpr double kThrustkN     = 200.0;
    static constexpr double kVacuumISP    = 300.0;

    PartProperty engine_{makeTestEngine(kEngineMass, kThrustkN, kVacuumISP)};
};

// Non-asparagus, 2 stages: verify remaining mass after stage 0 = stage 1 fullMass.
TEST_F(KinematicsTest, TwoStageNonAsparagusMassChain) {
    const double payloadMass = 2.0;
    const double fuelMass1   = 3.0;
    const double fuelMass0   = 6.0;

    auto s1 = makeNonAsparagusStage(payloadMass, engine_, 1, fuelMass1); // top
    auto s0 = makeNonAsparagusStage(s1.fullMass, engine_, 2, fuelMass0); // bottom

    RocketConfig cfg;
    cfg.stages = {s0, s1};

    std::vector<StageKinematics> kin;
    cfg.calcStageKinematics(kin);

    int sptr = 0;
    double currMass = s0.fullMass;

    // Stage 0 (one substage)
    double totFuel0 = s0.fullMass - s0.emptyMass;
    double burned0  = 1.0 * totFuel0;
    double tank0    = burned0 / 9.0;
    int detach0     = s0.engineMultiplicity;
    double mAfter0  = currMass - burned0 - detach0 * kEngineMass - tank0;

    // After stage 0 remaining mass should equal stage 1 fullMass
    EXPECT_NEAR(mAfter0, s1.fullMass, 1e-6);
    // m0 == fullMass
    EXPECT_NEAR(kin[0].m0, s0.fullMass, 1e-6);
    // mf == m0 - fuel
    EXPECT_NEAR(kin[0].mf, kin[0].m0 - burned0, 1e-6);
    EXPECT_GT(kin[0].burnTime, 0);

    // Stage 1 (one substage)
    double totFuel1 = s1.fullMass - s1.emptyMass;
    double burned1  = 1.0 * totFuel1;
    double tank1    = burned1 / 9.0;
    int detach1     = s1.engineMultiplicity;
    double mAfter1  = mAfter0 - burned1 - detach1 * kEngineMass - tank1;

    EXPECT_NEAR(kin[1].m0, s1.fullMass, 1e-6);
    EXPECT_NEAR(kin[1].mf, kin[1].m0 - burned1, 1e-6);
    EXPECT_GT(kin[1].burnTime, 0);
    // After last stage, remaining should be ≈ payloadMass
    EXPECT_NEAR(mAfter1, payloadMass, 1e-6);
    EXPECT_EQ(sptr + 2, static_cast<int>(kin.size()));
}

// Asparagus stage followed by non-asparagus: verify mass chain across all
// asparagus substages.
TEST_F(KinematicsTest, AsparagusThenNonAsparagusMassChain) {
    const double payloadMass = 2.0;
    const double fuelMass1   = 3.0; // top stage fuel

    // Top stage (non-asparagus)
    auto s1 = makeNonAsparagusStage(payloadMass, engine_, 1, fuelMass1);

    // Bottom stage (asparagus: 1 booster pair, innermost has engine)
    const int baseSymmetry       = 2;
    const int numSub             = 1;
    const std::uint16_t hasEngine = 0x01; // only innermost has engine
    const std::vector<double> fuelFractions = {0.6, 0.4};

    auto s0 = makeAsparagusStage(s1.fullMass, engine_,
                                 2,                // engineMultiplicity
                                 baseSymmetry,
                                 numSub,
                                 hasEngine,
                                 fuelFractions);

    RocketConfig cfg;
    cfg.stages = {s0, s1};

    std::vector<StageKinematics> kin;
    cfg.calcStageKinematics(kin);

    // Verify total substage count
    EXPECT_EQ(kin.size(), 3u); // 2 from s0 + 1 from s1

    double totFuel0   = s0.fullMass - s0.emptyMass;
    double fuelFrac0  = fuelFractions[0] * totFuel0;
    double fuelFrac1  = fuelFractions[1] * totFuel0;

    // Substage 0: outermost asparagus burns, innermost-detected boosters drop
    double tank0   = fuelFrac0 / 9.0;
    int    detach0 = ((hasEngine >> 0) & 0x1) * baseSymmetry;
    double mass0   = s0.fullMass - fuelFrac0;
    double remain0 = s0.fullMass - fuelFrac0 - detach0 * kEngineMass - tank0;

    EXPECT_NEAR(kin[0].m0, s0.fullMass, 1e-6);
    EXPECT_NEAR(kin[0].mf, mass0, 1e-6);
    EXPECT_GT(kin[0].burnTime, 0);

    // Substage 1 (core): burns remaining fuel, drops core engines
    double tank1   = fuelFrac1 / 9.0;
    int    detach1 = s0.engineMultiplicity;
    double remain1 = remain0 - fuelFrac1 - detach1 * kEngineMass - tank1;

    EXPECT_NEAR(kin[1].m0, remain0, 1e-6);
    EXPECT_NEAR(kin[1].mf, remain0 - fuelFrac1, 1e-6);
    EXPECT_GT(kin[1].burnTime, 0);

    // After all of stage 0, remaining mass == s1.fullMass
    EXPECT_NEAR(remain1, s1.fullMass, 1e-6);

    // Stage 1 (single substage)
    double totFuel1 = s1.fullMass - s1.emptyMass;
    double burned1  = 1.0 * totFuel1;
    double tankS1   = burned1 / 9.0;
    int    detachS1 = s1.engineMultiplicity;

    EXPECT_NEAR(kin[2].m0, s1.fullMass, 1e-6);
    EXPECT_NEAR(kin[2].mf, kin[2].m0 - burned1, 1e-6);
    EXPECT_GT(kin[2].burnTime, 0);
    EXPECT_NEAR(remain1 - burned1 - detachS1 * kEngineMass - tankS1, payloadMass, 1e-6);
}

// Drop-tank asparagus (hasEngine = 0): no booster engines detach.
TEST_F(KinematicsTest, DropTankAsparagus) {
    const double payloadMass     = 5.0;
    const int baseSymmetry       = 3;
    const int numSub             = 2;
    const std::uint16_t hasEngine = 0x00; // drop tanks, no booster engines
    const std::vector<double> fuelFractions = {0.5, 0.3, 0.2};

    auto s0 = makeAsparagusStage(payloadMass, engine_,
                                 1, baseSymmetry, numSub, hasEngine,
                                 fuelFractions);

    RocketConfig cfg;
    cfg.stages = {s0};

    std::vector<StageKinematics> kin;
    cfg.calcStageKinematics(kin);

    EXPECT_EQ(kin.size(), 3u); // numSub + 1
    double curr = s0.fullMass;
    double totFuel = s0.fullMass - s0.emptyMass;

    for (int sub = 0; sub <= numSub; ++sub) {
        double burned  = fuelFractions[sub] * totFuel;
        double tank    = burned / 9.0;
        int detach     = (sub == numSub) ? s0.engineMultiplicity
                        : ((hasEngine >> sub) & 0x1) * baseSymmetry;

        EXPECT_NEAR(kin[sub].m0, curr, 1e-6);
        EXPECT_NEAR(kin[sub].mf, curr - burned, 1e-6);
        EXPECT_GT(kin[sub].burnTime, 0);

        curr = curr - burned - detach * kEngineMass - tank;
    }

    // No booster engines ever detached (hasEngine=0), so only core drops.
    EXPECT_NEAR(curr, payloadMass, 1e-6);
}

// Edge: single non-asparagus stage should produce one kinematic entry.
TEST_F(KinematicsTest, SingleStageNonAsparagus) {
    auto s0 = makeNonAsparagusStage(10.0, engine_, 3, 8.0);

    RocketConfig cfg;
    cfg.stages = {s0};

    std::vector<StageKinematics> kin;
    cfg.calcStageKinematics(kin);

    ASSERT_EQ(kin.size(), 1u);
    EXPECT_NEAR(kin[0].m0, s0.fullMass, 1e-6);
    EXPECT_NEAR(kin[0].mf, s0.emptyMass, 1e-6);
    EXPECT_GT(kin[0].burnTime, 0);
}

// Edge: empty config produces empty kinematics, no crash.
TEST(KinematicsDeathTest, EmptyConfigNoCrash) {
    RocketConfig cfg;
    std::vector<StageKinematics> kin;
    EXPECT_NO_FATAL_FAILURE(cfg.calcStageKinematics(kin));
    EXPECT_TRUE(kin.empty());
}

TEST_F(KinematicsTest, MassComputation) {
    RocketConfig RC;
    PartProperty engine0{makeTestEngine(0.4, 100, 100)};
    PartProperty engine1{makeTestEngine(0.1, 100, 100)};
    std::vector<StageKinematics> kin;

    RC.stages.emplace_back();
    RC.stages.emplace_back();
    RC.stages[0].engine = &engine0;
    RC.stages[1].engine = &engine1;

    RC.stages[0].payloadMass = 0.1;
    RC.stages[1].payloadMass = 2.1;

    double fuelMass0 = 5;
    double fuelMass1 = 6;

    RC.stages[0].engineMultiplicity = 2;
    RC.stages[1].engineMultiplicity = 1;

    RC.recomputeMasses({fuelMass0, fuelMass1});

    double stage0_full_mass = RC.stages[0].engineMultiplicity * RC.stages[0].engine->mass;
    stage0_full_mass += fuelMass0 + fuelMass0 / 9.0;
    stage0_full_mass += RC.stages[0].payloadMass;

    double stage1_full_mass = RC.stages[1].engineMultiplicity * RC.stages[1].engine->mass;
    stage1_full_mass += fuelMass1 + fuelMass1 / 9.0;
    stage1_full_mass += RC.stages[1].payloadMass;

    double rocket0_mass = stage0_full_mass + stage1_full_mass;
    double rocket1_mass = stage1_full_mass;

    // Check fullMass
    EXPECT_NEAR(rocket0_mass, RC.stages[0].fullMass, 1e-5);
    EXPECT_NEAR(rocket1_mass, RC.stages[1].fullMass, 1e-5);
    // Check emptyMass
    EXPECT_NEAR(rocket0_mass - fuelMass0, RC.stages[0].emptyMass, 1e-5);
    EXPECT_NEAR(rocket1_mass - fuelMass1, RC.stages[1].emptyMass, 1e-5);
    // Check total rocket mass
    EXPECT_NEAR(rocket0_mass + rocket1_mass, RC.totalMass, 1e-5);
}

// ---------------------------------------------------------------------------
// Orbital data consistency
// ---------------------------------------------------------------------------

TEST(OrbitalDataTest, LANandLDN180Apart) {
    for (int i = 0; i < nBodies; ++i) {
        const Orbit& o = bodyTable[i].body->orbit;
        if (!o.parent) continue; // Kerbol has no orbit
        double diff = std::fmod(std::abs(o.LDN - o.LAN), 360.0f);
        EXPECT_NEAR(diff, 180.0f, 0.01f) << bodyTable[i].name;
    }
}

TEST(OrbitalDataTest, ApsidesConsistentWithSemiMajorAndEccentricity) {
    for (int i = 0; i < nBodies; ++i) {
        const Orbit& o = bodyTable[i].body->orbit;
        if (!o.parent) continue;
        EXPECT_NEAR(o.AP, o.a_semi * (1.0 + o.eccentricity), 1.0)
            << bodyTable[i].name;
        EXPECT_NEAR(o.PE, o.a_semi * (1.0 - o.eccentricity), 1.0)
            << bodyTable[i].name;
    }
}

TEST(OrbitalDataTest, GMConsistentWithSurfaceGravity) {
    for (int i = 0; i < nBodies; ++i) {
        const Body* b = bodyTable[i].body;
        double expected_from_sg = (b->surfaceGravity / 1000.0) * b->radius_km * b->radius_km;
        // surfaceGravity derived from geeASL * 9.80665; GM_km3s2 from Kittopia is authoritative.
        // Check consistency within 0.5%.
        EXPECT_NEAR(b->GM_km3s2, expected_from_sg, 1e-3)
            << bodyTable[i].name << ": GM_km3s2=" << b->GM_km3s2
            << " vs surfaceGravity-derived=" << expected_from_sg;
    }
}

// ---------------------------------------------------------------------------
// DeltaV calculators
// ---------------------------------------------------------------------------
//

const Body* allBodies[] = {
    &KspSystem::Mun,
    &KspSystem::Minmus,
    &KspSystem::Kerbin,
    &KspSystem::Dres,
    &KspSystem::Kerbol,
    &KspSystem::Duna,
    &KspSystem::Ike,
    &KspSystem::Jool,
    &KspSystem::Bop,
    &KspSystem::Tylo,
    &KspSystem::Pol,
    &KspSystem::Laythe,
    &KspSystem::Vall,
    &KspSystem::Moho,
    &KspSystem::Eeloo,
    &KspSystem::Gilly,
    &KspSystem::Eve
};
const Body* allPlanets[] = {
    &KspSystem::Kerbin,
    &KspSystem::Duna,
    &KspSystem::Jool,
    &KspSystem::Moho,
    &KspSystem::Eeloo,
    &KspSystem::Eve,
    &KspSystem::Dres,
};

TEST(DeltaV, naiveTakeoffLanding) {
    float orbit = 100;

    for (const auto* body : allBodies) {
        if (body->hasAtmosphere()) continue;
        float dv = naiveTakeoffLandingCost(body, orbit);
        
        // Compute expectation (double Hohmann)
        float dvExpected = 0;
        Orbit o1 = Orbit::elliptic(body, body->radius_km + orbit, body->radius_km);
        dvExpected += o1.v_periapsis(unit_mps);
        Orbit o2 = Orbit::circular(body, body->radius_km + orbit);
        dvExpected += o2.v_apoapsis(unit_mps) - o1.v_apoapsis(unit_mps);

        EXPECT_NEAR(dv, dvExpected, 1e-3);
    }
}

TEST(DeltaV, getHohmannOrbit) {
    // Kerbin -> Mun, Minmus
    for (const Body* sat : {&KspSystem::Mun, &KspSystem::Minmus}) {
        float R1 = KspSystem::Kerbin.radius_km + 80.0f;
        float R2 = sat->orbit.AP;
        auto H = getHohmannOrbit(&KspSystem::Kerbin, R1, R2);
        EXPECT_NEAR(H.PE, R1, 1e-3);
        EXPECT_NEAR(H.AP, R2, 1e-3);
        EXPECT_NEAR(H.a_semi, 0.5f * (H.AP + H.PE), 1e-3);
        EXPECT_NEAR(H.v_periapsis(unit_kmps), std::sqrt(KspSystem::Kerbin.GM_km3s2 * (2.0 / H.PE - 1.0 / H.a_semi)), 1e-3);
        EXPECT_NEAR(H.v_apoapsis(unit_kmps), std::sqrt(KspSystem::Kerbin.GM_km3s2 * (2.0 / H.AP - 1.0 / H.a_semi)), 1e-3);
    }

    // Jool -> Moons
    for (const Body* sat : {&KspSystem::Laythe, &KspSystem::Vall, &KspSystem::Bop, &KspSystem::Pol, &KspSystem::Tylo}) {
        float R1 = KspSystem::Jool.radius_km + 100.0f;
        float R2 = sat->orbit.AP;
        auto H = getHohmannOrbit(&KspSystem::Jool, R1, R2);
        EXPECT_NEAR(H.PE, R1, 1e-3);
        EXPECT_NEAR(H.AP, R2, 1e-3);
        EXPECT_NEAR(H.a_semi, 0.5f * (H.AP + H.PE), 1e-2);
        EXPECT_NEAR(H.v_periapsis(unit_kmps), std::sqrt(KspSystem::Jool.GM_km3s2 * (2.0 / H.PE - 1.0 / H.a_semi)), 1e-3);
        EXPECT_NEAR(H.v_apoapsis(unit_kmps), std::sqrt(KspSystem::Jool.GM_km3s2 * (2.0 / H.AP - 1.0 / H.a_semi)), 1e-3);
    }
}

TEST(DeltaV, escapeVelocity) {
    Orbit circ = Orbit::circular(&KspSystem::Kerbin, 80 + KspSystem::Kerbin.radius_km);
    float dv = escapeBurnCost(&KspSystem::Kerbin, 80);
    EXPECT_NEAR(dv, circ.v_apoapsis(unit_mps) * (std::numbers::sqrt2 - 1.0), 1e-3);
}


TEST(DeltaV, hohmannTransferCost) {
    // Earth and Moons
    for (const Body* moon : {&KspSystem::Mun, &KspSystem::Minmus}) {
        float start_alt = 80.0f;
        float Rmax = moon->orbit.AP;
        float Rmin = moon->orbit.PE;
        auto dvtrans = hohmannTransferCost(&KspSystem::Kerbin, moon, start_alt);

        // Compute expectation
        float dvExpected_max = 0;
        float dvExpected_min = 0;
        auto Hmax = getHohmannOrbit(&KspSystem::Kerbin, KspSystem::Kerbin.radius_km + start_alt, Rmax);
        auto Hmin = getHohmannOrbit(&KspSystem::Kerbin, KspSystem::Kerbin.radius_km + start_alt, Rmin);
        Orbit circ = Orbit::circular(&KspSystem::Kerbin, KspSystem::Kerbin.radius_km + start_alt);

        dvExpected_max = Hmax.v_periapsis(unit_mps) - circ.v_periapsis(unit_mps);
        dvExpected_min = Hmin.v_periapsis(unit_mps) - circ.v_periapsis(unit_mps);

        EXPECT_NEAR(dvtrans.max, dvExpected_max, 1e-3);
        EXPECT_NEAR(dvtrans.min, dvExpected_min, 1e-3);
    }
    for (const Body* moon : {&KspSystem::Tylo, &KspSystem::Vall, &KspSystem::Bop, &KspSystem::Pol, &KspSystem::Laythe}) {
        float start_alt = 100.0f;
        float Rmax = moon->orbit.AP;
        float Rmin = moon->orbit.PE;
        auto dvtrans = hohmannTransferCost(&KspSystem::Jool, moon, start_alt);

        // Compute expectation
        float dvExpected_max = 0;
        float dvExpected_min = 0;
        auto Hmax = getHohmannOrbit(&KspSystem::Jool, KspSystem::Jool.radius_km + start_alt, Rmax);
        auto Hmin = getHohmannOrbit(&KspSystem::Jool, KspSystem::Jool.radius_km + start_alt, Rmin);
        Orbit circ = Orbit::circular(&KspSystem::Jool, KspSystem::Jool.radius_km + start_alt);

        dvExpected_max = Hmax.v_periapsis(unit_mps) - circ.v_periapsis(unit_mps);
        dvExpected_min = Hmin.v_periapsis(unit_mps) - circ.v_periapsis(unit_mps);

        EXPECT_NEAR(dvtrans.max, dvExpected_max, 1e-3);
        EXPECT_NEAR(dvtrans.min, dvExpected_min, 1e-3);
    }

    // Planet -> Planet
    for (const Body* origin : allPlanets) {
        for (const Body* target : allPlanets) {
            if (origin == target) continue;
            auto dvtrans = hohmannTransferCost(origin, target);
    
            // Compute expectation
            auto Hmax = getHohmannOrbit(origin->orbit.parent, origin->orbit.PE, target->orbit.AP);
            auto Hmin = getHohmannOrbit(origin->orbit.parent, origin->orbit.AP, target->orbit.PE);
            float dv1, dv2;
            if (target->orbit.a_semi < origin->orbit.a_semi) {
                dv1 = std::abs(Hmax.v_apoapsis(unit_mps) - origin->orbit.v_periapsis(unit_mps));
                dv2 = std::abs(Hmin.v_apoapsis(unit_mps) - origin->orbit.v_apoapsis(unit_mps));
            } else {
                dv1 = std::abs(Hmax.v_periapsis(unit_mps) - origin->orbit.v_periapsis(unit_mps));
                dv2 = std::abs(Hmin.v_periapsis(unit_mps) - origin->orbit.v_apoapsis(unit_mps));
            }
            EXPECT_NEAR(dvtrans.max, std::max(dv1, dv2), 1e-3);
            EXPECT_NEAR(dvtrans.min, std::min(dv1, dv2), 1e-3);
        }
    }
}
