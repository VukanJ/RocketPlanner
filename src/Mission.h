#ifndef MISSION_H
#define MISSION_H

#include <atomic>
#include <limits>
#include <memory>
#include <vector>

#include "Calendar.h"
#include "DeltaV.h"
#include "kspConstants.h"

struct MissionPhase;

struct PorkchopPlot {
    static constexpr int minResolution = 16;
    static constexpr int maxResolution = 512;

    PorkchopPlot() = delete;
    PorkchopPlot(MissionPhase* phase = nullptr) : phase(phase) { }

    void generate();
    void init(KerbalDate launchStart);
    void render();
    void updateLaunchEnd();

    MissionPhase* phase = nullptr;

    bool winOpen = false;

    DateFormat launchStartInput { 0 };
    KerbalDate launchEndInput { 0 };
    DateFormat launchWindowDuration { 0, 0, 0, 0, 0 };
    int flightTimeMinDays = 50;
    int flightTimeMaxDays = 400;

    KerbalDate activeLaunchStart { 0 };
    KerbalDate activeLaunchEnd { 0 };
    int activeFlightTimeStartDays = 0;
    int activeFlightTimeEndDays = 0;

    int minDeltaVIndex = -1;
    int selectedIndex = -1;

    std::atomic<bool> calculated = false;
    std::atomic<float> progress = -1.0f;

    int colormapIndex = 0;
    int resolution = 96;
    int activeResolution = 0;
    float colorMaxDeltaV = 0.0f;
    bool useLogColorScale = true;
    float minDeltaV = std::numeric_limits<float>::infinity();
    float maxDeltaV = 0.0f;

    std::vector<float> deltaV;
    std::vector<float> deltaVLog;
};

struct Mission {
    const Body* originBody = &KspSystem::Kerbin;
    const Body* destinationBody = &KspSystem::Mun;
    bool oneWayTrip = false;
    bool apolloStyle = true;
    static void updateMissionSequence(std::vector<MissionPhase>& sequence, const Mission& mission, bool advanced);
};

struct MissionPhase {
    enum class Type {
        TAKEOFF,  // Implied circularization
        CIRCULARIZE_HYPERBOLIC,
        ESCAPE,
        HOHMANN_TRANSFER, // includes inclination correction
        ATMOSPHERIC_BREAKING,
        LANDING_PARACHUTES,
        INCLINATION_CORRECTION,
        ORBITAL_INSERTION, // Combines ESCAPE, INCLINATION_CORRECTION and HOHMANN_TRANSFER using Oberth effect
        LANDING,
        MINING,
        ORBITAL_REFUELING,
    } type = Type::TAKEOFF;

    const Body* refBody = nullptr;
    const Body* refBody2 = nullptr;
    float alt1 = 0;
    float alt2 = 0;
    DeltaVRange dv_range {};

    bool optional = true;
    bool prograde = true;

    std::unique_ptr<PorkchopPlot> porkchopPlot = nullptr;

    static MissionPhase takeoff(const Body* body, float alt) {
        return { Type::TAKEOFF, body, nullptr, alt, 0, false};
    }
    static MissionPhase hohmann(const Body* from, const Body* to, float fromAlt, float toAlt) {
        return { Type::HOHMANN_TRANSFER, from, to, fromAlt, toAlt, false};
    }
    static MissionPhase circularize_hyperbolic(const Body* from, const Body* to, float fromAlt, float toAlt) {
        return { Type::CIRCULARIZE_HYPERBOLIC, from, to, fromAlt, toAlt, false};
    }

    void updateDeltaV();
};

std::string PhaseToString(MissionPhase::Type type);


#endif // MISSION_H
