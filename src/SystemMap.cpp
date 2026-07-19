#include "SystemMap.h"
#include "WindowMission.h"
#include "kspConstants.h"
#include "DeltaV.h"
#include "utils.h"
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

namespace {
    constexpr int ORBIT_SAMPLES = 128;
    constexpr float DEG = M_PI / 180.0f;

    struct Color3 { float r, g, b; };

    ImU32 toImU32(Color3 c, float a = 1.0f) {
        return IM_COL32(int(c.r * 255), int(c.g * 255), int(c.b * 255), int(a * 255));
    }

    // Distinct color per body for visual identification
    Color3 bodyColor(const Body* b) {
        if (b == &KspSystem::Kerbol) { return {1.0f, 0.9f, 0.3f}; }
        if (b == &KspSystem::Moho)   { return {0.6f, 0.5f, 0.4f}; }
        if (b == &KspSystem::Eve)    { return {0.7f, 0.3f, 0.8f}; }
        if (b == &KspSystem::Gilly)  { return {0.5f, 0.2f, 0.2f}; }
        if (b == &KspSystem::Kerbin) { return {0.2f, 0.4f, 0.9f}; }
        if (b == &KspSystem::Mun)    { return {0.5f, 0.5f, 0.5f}; }
        if (b == &KspSystem::Minmus) { return {0.3f, 0.8f, 0.8f}; }
        if (b == &KspSystem::Duna)   { return {0.8f, 0.3f, 0.1f}; }
        if (b == &KspSystem::Ike)    { return {0.4f, 0.4f, 0.4f}; }
        if (b == &KspSystem::Dres)   { return {0.6f, 0.5f, 0.3f}; }
        if (b == &KspSystem::Jool)   { return {0.2f, 0.7f, 0.2f}; }
        if (b == &KspSystem::Laythe) { return {0.3f, 0.6f, 0.7f}; }
        if (b == &KspSystem::Vall)   { return {0.5f, 0.7f, 0.9f}; }
        if (b == &KspSystem::Tylo)   { return {0.7f, 0.7f, 0.7f}; }
        if (b == &KspSystem::Bop)    { return {0.4f, 0.2f, 0.5f}; }
        if (b == &KspSystem::Pol)    { return {0.7f, 0.6f, 0.4f}; }
        if (b == &KspSystem::Eeloo)  { return {0.9f, 0.9f, 0.9f}; }
        return {1.0f, 1.0f, 1.0f};
    }

    // Sample an orbit as 3D world-space points.
    // Applies the same LAN → inclination → AoP rotation as Orbit.cpp,
    // but uses the polar form r(θ) to trace the full ellipse rather
    // than solving for a single position from meanAnomaly.
    void sampleOrbit(const Orbit& orbit, std::vector<glm::vec3>& pts,
                     float startAngle = 0.0f, float endAngle = 2.0f * M_PI) {
        pts.resize(ORBIT_SAMPLES);
        float a = orbit.a_semi;
        float e = orbit.eccentricity;

        glm::mat4 LAN_rot = glm::rotate(glm::mat4(1.0f), -orbit.LAN * DEG, glm::vec3(0, 1, 0));
        glm::mat4 inc_rot = glm::rotate(glm::mat4(1.0f), -orbit.inclination * DEG, glm::vec3(1, 0, 0));
        glm::mat4 AoP_rot = glm::rotate(glm::mat4(1.0f), -orbit.argumentOfPeriapsis * DEG, glm::vec3(0, 1, 0));
        glm::mat4 xform = AoP_rot * inc_rot * LAN_rot;

        for (int i = 0; i < ORBIT_SAMPLES; ++i) {
            float theta = startAngle + (endAngle - startAngle) * i / ORBIT_SAMPLES;
            float r = a * (1.0f - e * e) / (1.0f + e * cosf(theta));
            glm::vec4 p(r * cosf(theta), 0.0f, r * sinf(theta), 1.0f);
            pts[i] = glm::vec3(xform * p);
        }
    }

    // Project a 3D world-space point to 2D ImGui window coordinates via
    // the combined view-projection matrix. Returns z in NDC for depth checks.
    glm::vec3 project(const glm::vec3& wp, const glm::mat4& vp, ImVec2 origin, ImVec2 sz) {
        glm::vec4 c = vp * glm::vec4(wp, 1.0f);
        if (c.w <= 0.0f) { return {-1, -1, -1}; }
        float ndcX = c.x / c.w;
        float ndcY = c.y / c.w;
        float sx = (ndcX + 1.0f) * 0.5f * sz.x + origin.x;
        float sy = (1.0f - ndcY) * 0.5f * sz.y + origin.y;
        return {sx, sy, c.z / c.w};
    }

    // Compute per-segment blend factor for the trail brighten effect.
    // Returns t ∈ [0, 1] where t=1 means recently crossed (brightest).
    std::vector<float> computeOrbitBrightness(float meanAnomaly) {
        std::vector<float> t(ORBIT_SAMPLES);
        constexpr float DECAY = 2.0f;
        for (int i = 0; i < ORBIT_SAMPLES; ++i) {
            float theta = 2.0f * M_PI * i / ORBIT_SAMPLES;
            float delta = fmodf(meanAnomaly - theta, 2.0f * M_PI);
            if (delta < 0.0f) { delta += 2.0f * M_PI; }
            t[i] = expf(-DECAY * delta);
        }
        return t;
    }

    // Draw an orbit as individual line segments, culling segments where
    // either endpoint is behind the camera to prevent wrap-around artifacts.
    void drawOrbitLine(ImDrawList* dl, const std::vector<glm::vec3>& pts,
                       const glm::mat4& vp, ImVec2 origin, ImVec2 sz,
                       ImU32 color, float thickness, bool closed = true)
    {
        std::vector<ImVec2> screen(pts.size());
        std::vector<bool> visible(pts.size());
        for (size_t i = 0; i < pts.size(); ++i) {
            glm::vec3 s = project(pts[i], vp, origin, sz);
            screen[i] = {s.x, s.y};
            visible[i] = (s.z > -1.0f && s.z < 1.0f);
        }
        size_t count = closed ? pts.size() : pts.size() - 1;
        for (size_t i = 0; i < count; ++i) {
            size_t j = (i + 1) % pts.size();
            if (visible[i] && visible[j]) {
                dl->AddLine(screen[i], screen[j], color, thickness);
            }
        }
    }

    // Overload: per-segment brightness for the trail effect.
    // Blends between base color (t=0) and a brightened version (t=1).
    void drawOrbitLine(ImDrawList* dl, const std::vector<glm::vec3>& pts,
                       const glm::mat4& vp, ImVec2 origin, ImVec2 sz,
                       Color3 base, const std::vector<float>& brightness, float thickness)
    {
        Color3 bright = { std::min(base.r + 0.4f, 1.0f),
                          std::min(base.g + 0.4f, 1.0f),
                          std::min(base.b + 0.4f, 1.0f) };
        std::vector<ImVec2> screen(pts.size());
        std::vector<bool> visible(pts.size());
        for (size_t i = 0; i < pts.size(); ++i) {
            glm::vec3 s = project(pts[i], vp, origin, sz);
            screen[i] = {s.x, s.y};
            visible[i] = (s.z > -1.0f && s.z < 1.0f);
        }
        for (size_t i = 0; i < pts.size(); ++i) {
            size_t j = (i + 1) % pts.size();
            if (visible[i] && visible[j]) {
                float t = std::min(brightness[i], brightness[j]);
                Color3 c = { base.r + (bright.r - base.r) * t,
                             base.g + (bright.g - base.g) * t,
                             base.b + (bright.b - base.b) * t };
                dl->AddLine(screen[i], screen[j], toImU32(c), thickness);
            }
        }
    }
}

// Rebuild cached 3D orbit data. Only called when the mission changes.
void SystemMap::rebuildCache(const Mission& mission, int64_t seconds) {
    planetCache.clear();

    // Cache all planet orbits around Kerbol
    for (auto& entry : bodyTable) {
        const Body* body = entry.body;
        if (body->orbit.parent != &KspSystem::Kerbol) { continue; }
        if (body->orbit.a_semi <= 0) { continue; }

        CachedOrbit co;
        co.name = body->name;
        Color3 bc = bodyColor(body);
        co.r = bc.r; co.g = bc.g; co.b = bc.b;
        sampleOrbit(body->orbit, co.points);
        float advMeanAnomaly = fmodf(body->orbit.meanAnomaly + 2.0f * M_PI * (static_cast<float>(seconds) / body->orbit.period()), 2.0f * M_PI);
        co.brightness = computeOrbitBrightness(advMeanAnomaly);
        auto [pos, vel] = get_local_future_position(body->orbit, seconds);
        co.worldPos = glm::vec3(pos.x(), pos.y(), pos.z());
        planetCache.push_back(std::move(co));
    }

    cachedOrigin = mission.originBody;
    cachedDest = mission.destinationBody;
    cachedDateSeconds = seconds;
}

// Renders the system map as a floating ImGui window. Shows all bodies
// orbiting Kerbol with their orbits, positions, and any Hohmann transfer
// arc for the currently selected mission.
void SystemMap::render(const Mission& mission, int64_t seconds) {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float panelWidth = 420.0f;
    ImGui::SetNextWindowPos(ImVec2(panelWidth, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x - panelWidth, displaySize.y), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("System Map", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImVec2 wpos = ImGui::GetCursorScreenPos();
    ImVec2 wsz = ImGui::GetContentRegionAvail();
    if (wsz.x < 10 || wsz.y < 10) {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Invisible button captures mouse input for the map area
    ImGui::InvisibleButton("##maparea", wsz);
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    // Scroll to zoom, drag to rotate camera
    if (hovered) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f) {
            distance *= (1.0f - io.MouseWheel * 0.1f);
            if (distance < 100.0f) { distance = 100.0f; }
        }
    }
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        azimuth += d.x * 0.005f;
        elevation += d.y * 0.005f;
        if (elevation > M_PI * 0.49f) { elevation = M_PI * 0.49f; }
        if (elevation < -M_PI * 0.49f) { elevation = -M_PI * 0.49f; }
    }

    // Rebuild cache when mission selection or date changes
    if (planetCache.empty() || cachedOrigin != mission.originBody || cachedDest != mission.destinationBody || cachedDateSeconds != seconds) {
        rebuildCache(mission, seconds);
    }

    // Spherical camera → lookAt view matrix, perspective projection
    glm::vec3 camPos(
        distance * cosf(elevation) * sinf(azimuth),
        distance * sinf(elevation),
        distance * cosf(elevation) * cosf(azimuth)
    );
    glm::mat4 view = glm::lookAtLH(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    float aspect = wsz.x / wsz.y;
    glm::mat4 proj = glm::perspectiveLH(glm::radians(fov), aspect, distance * 0.001f, distance * 200.0f);
    glm::mat4 vp = proj * view;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(wpos, ImVec2(wpos.x + wsz.x, wpos.y + wsz.y), IM_COL32(10, 10, 15, 255));

    // Draw each cached planet orbit and compute its screen position
    std::vector<ImVec2> bodyScreenPos;
    std::vector<const CachedOrbit*> visibleBodies;

    for (auto& co : planetCache) {
        Color3 c = { co.r, co.g, co.b };
        drawOrbitLine(dl, co.points, vp, wpos, wsz, c, co.brightness, 1.0f);

        glm::vec3 sp = project(co.worldPos, vp, wpos, wsz);
        if (sp.z >= -1.0f && sp.z <= 1.0f) {
            bodyScreenPos.push_back({sp.x, sp.y});
            visibleBodies.push_back(&co);
        }
    }

    // Draw debug orbits (transfer orbits, etc.)
    for (auto& dbg : debugOrbits) {
        drawOrbitDebug(dbg.orbit, dl, vp, wpos, wsz, dbg.color, dbg.thickness, dbg.startAngle, dbg.endAngle);
    }

    // Draw central star (Kerbol) at the origin
    {
        glm::vec3 sp = project(glm::vec3(0, 0, 0), vp, wpos, wsz);
        if (sp.z >= -1.0f && sp.z <= 1.0f) {
            dl->AddCircleFilled(ImVec2(sp.x, sp.y), 8.0f, IM_COL32(255, 230, 77, 255));
            dl->AddText(ImVec2(sp.x + 12, sp.y - 6), IM_COL32(255, 255, 200, 255), "Kerbol");
        }
    }

    // Draw planet markers and labels
    for (size_t i = 0; i < visibleBodies.size(); ++i) {
        dl->AddCircleFilled(bodyScreenPos[i], 4.0f, toImU32({visibleBodies[i]->r, visibleBodies[i]->g, visibleBodies[i]->b}));
        dl->AddText(ImVec2(bodyScreenPos[i].x + 8, bodyScreenPos[i].y - 6),
                    IM_COL32(220, 220, 220, 255), visibleBodies[i]->name);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void SystemMap::addDebugOrbit(const DebugOrbit& dbg) {
    debugOrbits.push_back(dbg);
}

void SystemMap::clearDebugOrbits() {
    debugOrbits.clear();
}

void SystemMap::drawOrbitDebug(const Orbit& orbit, ImDrawList* dl,
                               const glm::mat4& vp, ImVec2 origin, ImVec2 sz,
                               ImU32 color, float thickness,
                               float startAngle, float endAngle)
{
    // Draw the orbit arc
    std::vector<glm::vec3> pts;
    sampleOrbit(orbit, pts, startAngle, endAngle);
    drawOrbitLine(dl, pts, vp, origin, sz, color, thickness, false);

    float a = orbit.a_semi;
    float e = orbit.eccentricity;
    float b = a * sqrtf(1.0f - e * e);
    float ae = a * e;
    float r_p = a * (1.0f - e);
    float r_a = a * (1.0f + e);

    // Build the same rotation chain as sampleOrbit
    glm::mat4 LAN_rot = glm::rotate(glm::mat4(1.0f), -orbit.LAN * DEG, glm::vec3(0, 1, 0));
    glm::mat4 inc_rot = glm::rotate(glm::mat4(1.0f), -orbit.inclination * DEG, glm::vec3(1, 0, 0));
    glm::mat4 AoP_rot = glm::rotate(glm::mat4(1.0f), -orbit.argumentOfPeriapsis * DEG, glm::vec3(0, 1, 0));
    glm::mat4 xform = AoP_rot * inc_rot * LAN_rot;

    // Helper: rotate a perifocal point to world space and project to screen
    auto toScreen = [&](float x, float y, float z) -> glm::vec3 {
        glm::vec4 p = xform * glm::vec4(x, y, z, 1.0f);
        return project(glm::vec3(p), vp, origin, sz);
    };

    // Helper: draw a dashed line between two screen-space points
    auto drawDashed = [&](ImVec2 from, ImVec2 to, ImU32 c, float thickness, float dashLen = 8.0f, float gapLen = 5.0f) {
        float dx = to.x - from.x;
        float dy = to.y - from.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1.0f) return;
        float nx = dx / len;
        float ny = dy / len;
        float pos = 0.0f;
        while (pos < len) {
            float end = std::min(pos + dashLen, len);
            dl->AddLine(ImVec2(from.x + nx * pos, from.y + ny * pos),
                        ImVec2(from.x + nx * end, from.y + ny * end),
                        c, thickness);
            pos = end + gapLen;
        }
    };

    // Helper: draw a labeled dot
    auto drawDot = [&](const glm::vec3& sp, ImU32 c, const char* label, float radius = 4.0f) {
        if (sp.z < -1.0f || sp.z > 1.0f) return;
        dl->AddCircleFilled(ImVec2(sp.x, sp.y), radius, c);
        dl->AddText(ImVec2(sp.x + 8, sp.y - 6), c, label);
    };

    // Key positions in perifocal frame
    struct Annotation {
        float x, y, z;
        const char* label;
        ImU32 color;
        float dotRadius;
    };

    // Perifocal positions: x = radial (toward periapsis), z = transverse
    // Periapsis is at +x, apoapsis at -x
    // Foci: primary at origin, secondary at (-2ae, 0, 0)

    // Points to annotate
    std::vector<Annotation> annotations = {
        { r_p,  0, 0, "PE",      IM_COL32(100, 255, 100, 255), 5.0f },
        {-r_a,  0, 0, "AP",      IM_COL32(255, 100, 100, 255), 5.0f },
        {-2*ae, 0, 0, "F'",      IM_COL32(200, 200, 200, 255), 3.0f },
    };

    for (auto& a : annotations) {
        glm::vec3 sp = toScreen(a.x, a.y, a.z);
        drawDot(sp, a.color, a.label, a.dotRadius);
    }

    // Semi-major axis line: from AP (-r_a, 0, 0) to PE (r_p, 0, 0)
    {
        glm::vec3 spA = toScreen(-r_a, 0, 0);
        glm::vec3 spB = toScreen( r_p, 0, 0);
        if (spA.z >= -1.0f && spA.z <= 1.0f && spB.z >= -1.0f && spB.z <= 1.0f) {
            drawDashed({spA.x, spA.y}, {spB.x, spB.y}, IM_COL32(180, 180, 180, 180), 1.0f);
            float mx = (spA.x + spB.x) * 0.5f;
            float my = (spA.y + spB.y) * 0.5f;
            char buf[32];
            snprintf(buf, sizeof(buf), "a=%.0f", a);
            dl->AddText(ImVec2(mx + 4, my - 14), IM_COL32(180, 180, 180, 220), buf);
        }
    }
}
