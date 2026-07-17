#ifndef SYSTEM_MAP_H
#define SYSTEM_MAP_H

#include "Orbit.h"
#include <glm/glm.hpp>
#include <vector>

struct Mission;

struct CachedOrbit {
    std::vector<glm::vec3> points;
    std::vector<float> brightness;
    glm::vec3 worldPos;
    float r, g, b;
    const char* name;
};

class SystemMap {
public:
    void render(const Mission& mission, int64_t seconds);

private:
    float azimuth = 0.4f;
    float elevation = 0.6f;
    float distance = 8000000.0f;
    float fov = 45.0f;

    // Cached planet orbit data (rebuilt only when mission changes)
    std::vector<CachedOrbit> planetCache;
    const Body* cachedOrigin = nullptr;
    const Body* cachedDest = nullptr;
    int64_t cachedDateSeconds = -1;

    // Cached transfer arcs
    Orbit cachedHoh1 = {};
    Orbit cachedHoh2 = {};

    void rebuildCache(const Mission& mission, int64_t seconds);
};

#endif // SYSTEM_MAP_H
