#ifndef WINDOW_MISSION
#define WINDOW_MISSION

#include "DeltaV.h"
#include "kspConstants.h"
#include <vector>

struct Mission {
    const Body* originBody = &KspSystem::Kerbin;
    const Body* destinationBody = &KspSystem::Mun;
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

    bool active = true;
    
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

class WindowMission {
public:
    WindowMission();
    Mission mission;

    bool render();
    void updateMissionSequence();

    enum class InputPhase { FromTo, Sequence } input_phase = WindowMission::InputPhase::FromTo;

    std::vector<MissionPhase> msequence;
};

#endif // WINDOW_MISSION
