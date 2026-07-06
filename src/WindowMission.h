#ifndef WINDOW_MISSION
#define WINDOW_MISSION

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
    LANDING,
    MINING,
    ORBITAL_REFUELING,
    //GRAVITY_ASSIST
};

struct MissionPhase {
    MissionPhaseType type = MissionPhaseType::TAKEOFF;
    const Body* refBody = nullptr;
    const Body* refBody2 = nullptr;
    float orbitAltitude = 0;
    bool optional = true;
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
