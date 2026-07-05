#include "WindowMission.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"

namespace {
    bool renderBodyCombo(const char* label, const Body*& selected) {
        bool changed = false;
        const char* preview = "None";
        for (auto& entry : bodyTable) {
            if (entry.body == selected) {
                preview = entry.name;
                break;
            }
        }
        if (ImGui::BeginCombo(label, preview)) {
            for (auto& entry : bodyTable) {
                bool isSelected = (entry.body == selected);
                if (ImGui::Selectable(entry.name, isSelected)) {
                    selected = entry.body;
                    changed = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (selected) {
            ImGui::BulletText("Surface g: %.2f m/s²", selected->surfaceGravity);
            ImGui::BulletText("Radius: %.0f km", selected->radius_km);
            if (selected->seaLevel_atm > 0.0f) {
                ImGui::BulletText("Atmosphere: %.4f atm", selected->seaLevel_atm);
                ImGui::BulletText("Atm height: %.0f km", selected->atmHeight_km);
            }
            else {
                ImGui::BulletText("No atmosphere");
            }
        }
        return changed;
    }
}

WindowMission::WindowMission() {
    updateMissionSequence();
}

bool WindowMission::render() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Configure Mission", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove);

    bool changed = false;

    ImGui::Text("Origin");
    changed |= renderBodyCombo("##origin", mission.originBody);

    ImGui::Separator();

    ImGui::Text("Destination");
    changed |= renderBodyCombo("##dest", mission.destinationBody);

    ImGui::Separator();

    changed |= ImGui::Checkbox("One-way trip", &mission.oneWayTrip);
    ImGui::SameLine();
    ImGui::BeginDisabled(mission.oneWayTrip);
    changed |= ImGui::Checkbox("Apollo-style", &mission.apolloStyle);
    ImGui::EndDisabled();

    bool next = false;
    if (ImGui::Button("Design Rocket")) {
        next = true;
    }

    if (changed) {
        updateMissionSequence();
    }

    ImGui::End();

    return next;
}

void WindowMission::updateMissionSequence() {
    msequence.clear();

    auto fromTo = [this](const Body* from, const Body* to) {
        msequence.push_back(MissionPhase { MissionPhaseType::TAKEOFF, from, nullptr, 0 });
        msequence.push_back(MissionPhase { MissionPhaseType::CIRCULARIZE, from, nullptr, from->atmHeight_km + 10 });
        if (from == to) {
            if (to->atmHeight_km > 0) {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, from, nullptr, from->atmHeight_km + 10 });
            }
            else {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING, from, nullptr, to->atmHeight_km + 10 });
            }
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::HOHMANN_TRANSFER, from, to, 0 });
        }
        if (to->atmHeight_km > 0) {
            msequence.push_back(MissionPhase { MissionPhaseType::ATMOSPHERIC_BREAKING, to, nullptr, to->atmHeight_km + 10 });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, to, nullptr, to->atmHeight_km + 10 });
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::CIRCULARIZE, to, nullptr, to->atmHeight_km + 10 });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING, to, nullptr, to->atmHeight_km + 10 });
        }
    };
    fromTo(mission.originBody, mission.destinationBody);
    if (!mission.oneWayTrip) {
        fromTo(mission.destinationBody, mission.originBody);
    }
}


