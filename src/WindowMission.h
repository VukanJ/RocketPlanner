#ifndef WINDOW_MISSION
#define WINDOW_MISSION

#include "DeltaV.h"
#include "LambertSolver.h"
#include "SystemMap.h"
#include "kspConstants.h"
#include <limits>
#include <vector>

struct Mission {
    const Body* originBody = &KspSystem::Kerbin;
    const Body* destinationBody = &KspSystem::Duna;
    bool oneWayTrip = false;
    bool apolloStyle = true;

    float initialApoapsis = 80;
    float destinationApoapsis = 10;
};

enum class MissionPhaseType {
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
};

struct MissionPhase {
    MissionPhaseType type = MissionPhaseType::TAKEOFF;
    const Body* refBody = nullptr;
    const Body* refBody2 = nullptr;
    float alt1 = 0;
    float alt2 = 0;
    DeltaVRange dv_range;

    bool optional = true;
    bool prograde = true;

    static MissionPhase takeoff(const Body* body, float alt) {
        return { MissionPhaseType::TAKEOFF, body, nullptr, alt, 0, false};
    }
    static MissionPhase hohmann(const Body* from, const Body* to, float fromAlt, float toAlt) {
        return { MissionPhaseType::HOHMANN_TRANSFER, from, to, fromAlt, toAlt, false};
    }
    static MissionPhase circularize_hyperbolic(const Body* from, const Body* to, float fromAlt, float toAlt) {
        return { MissionPhaseType::CIRCULARIZE_HYPERBOLIC, from, to, fromAlt, toAlt, false};
    }

    void updateDeltaV();
};

struct Date {
    int year = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

    Date(int64_t seconds);
    int64_t toSeconds() const;
};

struct PorkchopPlot {
    static constexpr int minResolution = 16;
    static constexpr int maxResolution = 512;

    bool isOpen = false;
    Date launchStart { 0 };
    Date launchEnd { 0 };
    int flightTimeStartDays = 50;
    int flightTimeEndDays = 400;
    int resolution = 96;
    int calculatedResolution = 0;
    Date calculatedLaunchStart { 0 };
    Date calculatedLaunchEnd { 0 };
    int calculatedFlightTimeStartDays = 0;
    int calculatedFlightTimeEndDays = 0;
    int colormapIndex = 0;
    std::vector<float> deltaV;
    float minDeltaV = std::numeric_limits<float>::infinity();
    float maxDeltaV = 0.0f;
    float colorMaxDeltaV = 0.0f;
    int cheapestLaunchIndex = -1;
    int cheapestFlightIndex = -1;
    bool calculated = false;
};

class WindowMission {
public:
    WindowMission();
    Mission mission;

    bool render();
    void updateMissionSequence();
    void calcLaunchWindow();
    void showTransferOrbit(float launchSeconds, float flightSeconds);
    bool renderTimeInput();
    void renderPorkchopPlot();
    void generatePorkchopPlot();

    enum class InputPhase { FromTo, Sequence } input_phase = WindowMission::InputPhase::FromTo;

    std::vector<MissionPhase> msequence;
    bool advanced = true;
    bool launchDateOptim = false;
    SystemMap systemMap;

    Date currentDate { 0 };
    LambertSolver transfer_solver;
    bool update_solver = false;
    PorkchopPlot porkchopPlot;
};

#endif // WINDOW_MISSION
