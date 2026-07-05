#ifndef WINDOW_MISSION
#define WINDOW_MISSION

#include "kspConstants.h"

struct Mission {
    const Body* originBody = &KspSystem::Kerbin;
    const Body* destinationBody = &KspSystem::Mun;
    bool oneWayTrip = false;
    bool apolloStyle = true;

    float initialApoapsis = 80;
    float destinationApoapsis = 10;
};

class WindowMission {
public:
    Mission mission;

    bool render();
};

#endif // WINDOW_MISSION
