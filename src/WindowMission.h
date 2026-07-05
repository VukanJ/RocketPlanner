#ifndef WINDOW_MISSION
#define WINDOW_MISSION

#include "kspConstants.h"

struct Mission {
    Body originBody = KspSystem::Kerbin;
    Body destinationBody = KspSystem::Mun;
    bool oneWayTrip = false;
    bool apolloStyle = true;

    float initialApoapsis = 80;
    float destinationApoapsis = 10;
};

class WindowMission {
public:

    void render();

    void onWindowResized(int width, int height);

    int windowWidth = 1280;
    int windowHeight = 720;
};

#endif // WINDOW_MISSION
