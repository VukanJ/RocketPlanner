#ifndef SYSTEM_MAP_H
#define SYSTEM_MAP_H

#include "Orbit.h"
#include <glm/glm.hpp>
#include <vector>

struct ImDrawList;
struct ImVec2;
using ImU32 = unsigned int;

struct Mission;

struct CachedOrbit {
    std::vector<glm::vec3> points;
    std::vector<float> brightness;
    glm::vec3 worldPos;
    float r, g, b;
    const char* name;
};

struct DebugOrbit {
    Orbit orbit;
    ImU32 color;
    float thickness = 2.0f;
    float startAngle = 0.0f;
    float endAngle = 2.0f * 3.14159265f;
};

class SystemMap {
public:
    void render(const Mission& mission, int64_t seconds);
    void drawOrbitDebug(const Orbit& orbit, ImDrawList* dl,
                        const glm::mat4& vp, ImVec2 origin, ImVec2 sz,
                        ImU32 color, float thickness = 2.0f,
                        float startAngle = 0.0f, float endAngle = 6.28318530f);
    void addDebugOrbit(const DebugOrbit& dbg);
    void clearDebugOrbits();

private:
    float azimuth = 0.4f;
    float elevation = 0.6f;
    float distance = 8000000.0f;
    float fov = 45.0f;

    // Cached planet orbit data (rebuilt only when mission changes)
    std::vector<CachedOrbit> planetCache;
    std::vector<DebugOrbit> debugOrbits;
    const Body* cachedOrigin = nullptr;
    const Body* cachedDest = nullptr;
    int64_t cachedDateSeconds = -1;

    void rebuildCache(const Mission& mission, int64_t seconds);
};

#endif // SYSTEM_MAP_H
