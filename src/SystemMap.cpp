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
    void sampleOrbit(const Orbit& orbit, std::vector<glm::vec3>& pts) {
        pts.resize(ORBIT_SAMPLES);
        float a = orbit.a_semi;
        float e = orbit.eccentricity;

        glm::mat4 LAN_rot = glm::rotate(glm::mat4(1.0f), -orbit.LAN * DEG, glm::vec3(0, 1, 0));
        glm::mat4 inc_rot = glm::rotate(glm::mat4(1.0f), -orbit.inclination * DEG, glm::vec3(1, 0, 0));
        glm::mat4 AoP_rot = glm::rotate(glm::mat4(1.0f), -orbit.argumentOfPeriapsis * DEG, glm::vec3(0, 1, 0));
        glm::mat4 xform = AoP_rot * inc_rot * LAN_rot;

        for (int i = 0; i < ORBIT_SAMPLES; ++i) {
            float theta = 2.0f * M_PI * i / ORBIT_SAMPLES;
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

    // Draw an orbit as individual line segments, culling segments where
    // either endpoint is behind the camera to prevent wrap-around artifacts.
    void drawOrbitLine(ImDrawList* dl, const std::vector<glm::vec3>& pts,
                       const glm::mat4& vp, ImVec2 origin, ImVec2 sz,
                       ImU32 color, float thickness)
    {
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
                dl->AddLine(screen[i], screen[j], color, thickness);
            }
        }
    }
}

// Renders the system map as a floating ImGui window. Shows all bodies
// orbiting Kerbol with their orbits, positions, and any Hohmann transfer
// arc for the currently selected mission.
void SystemMap::render(const Mission& mission) {
    ImGui::Checkbox("System Map", &show);
    if (!show) { return; }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin("System Map", &show)) {
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }

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

    // Spherical camera → lookAt view matrix, perspective projection
    glm::vec3 camPos(
        distance * cosf(elevation) * sinf(azimuth),
        distance * sinf(elevation),
        distance * cosf(elevation) * cosf(azimuth)
    );
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    float aspect = wsz.x / wsz.y;
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, distance * 0.001f, distance * 200.0f);
    glm::mat4 vp = proj * view;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(wpos, ImVec2(wpos.x + wsz.x, wpos.y + wsz.y), IM_COL32(10, 10, 15, 255));

    std::vector<glm::vec3> pts;
    std::vector<ImVec2> bodyScreenPos;
    std::vector<const Body*> bodyList;

    // Draw each planet's orbit and compute its screen position
    for (auto& entry : bodyTable) {
        const Body* body = entry.body;
        if (body->orbit.parent != &KspSystem::Kerbol) { continue; }
        if (body->orbit.a_semi <= 0) { continue; }

        Color3 c = bodyColor(body);
        sampleOrbit(body->orbit, pts);
        drawOrbitLine(dl, pts, vp, wpos, wsz, toImU32(c, 0.4f), 1.0f);

        // get_local_position solves Kepler's equation to find the body's
        // position on its orbit at the current mean anomaly (t=0)
        auto [pos, vel] = get_local_position(body->orbit);
        glm::vec3 wp(pos.x(), pos.y(), pos.z());

        glm::vec3 sp = project(wp, vp, wpos, wsz);
        if (sp.z >= -1.0f && sp.z <= 1.0f) {
            bodyScreenPos.push_back({sp.x, sp.y});
            bodyList.push_back(body);
        }
    }

    // Draw central star (Kerbol) at the origin
    {
        glm::vec3 sp = project(glm::vec3(0, 0, 0), vp, wpos, wsz);
        if (sp.z >= -1.0f && sp.z <= 1.0f) {
            Color3 c = bodyColor(&KspSystem::Kerbol);
            dl->AddCircleFilled(ImVec2(sp.x, sp.y), 8.0f, toImU32(c));
            dl->AddText(ImVec2(sp.x + 12, sp.y - 6), IM_COL32(255, 255, 200, 255), "Kerbol");
        }
    }

    // Draw planet markers and labels
    for (size_t i = 0; i < bodyList.size(); ++i) {
        Color3 c = bodyColor(bodyList[i]);
        dl->AddCircleFilled(bodyScreenPos[i], 4.0f, toImU32(c));
        dl->AddText(ImVec2(bodyScreenPos[i].x + 8, bodyScreenPos[i].y - 6),
                    IM_COL32(220, 220, 220, 255), bodyList[i]->name);
    }

    // Draw Hohmann transfer arcs between the selected origin and destination
    if (mission.originBody && mission.destinationBody &&
        mission.originBody->orbit.parent == &KspSystem::Kerbol &&
        mission.destinationBody->orbit.parent == &KspSystem::Kerbol)
    {
        Orbit hoh = getHohmannOrbit(&KspSystem::Kerbol,
                                    mission.originBody->orbit.PE,
                                    mission.destinationBody->orbit.AP);
        if (hoh.a_semi > 0) {
            sampleOrbit(hoh, pts);
            drawOrbitLine(dl, pts, vp, wpos, wsz, IM_COL32(0, 255, 100, 200), 2.0f);
        }
        Orbit hoh2 = getHohmannOrbit(&KspSystem::Kerbol,
                                     mission.originBody->orbit.AP,
                                     mission.destinationBody->orbit.PE);
        if (hoh2.a_semi > 0) {
            sampleOrbit(hoh2, pts);
            drawOrbitLine(dl, pts, vp, wpos, wsz, IM_COL32(100, 255, 0, 200), 2.0f);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
