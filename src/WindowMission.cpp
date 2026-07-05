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

WindowMission::WindowMission() { }

bool WindowMission::render() {
    if (input_phase == InputPhase::FromTo) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("Configure Mission", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove);

        ImGui::Text("Origin");
        renderBodyCombo("##origin", mission.originBody);

        ImGui::Separator();

        ImGui::Text("Destination");
        renderBodyCombo("##dest", mission.destinationBody);

        ImGui::Separator();

        ImGui::Checkbox("One-way trip", &mission.oneWayTrip);
        ImGui::SameLine();
        ImGui::BeginDisabled(mission.oneWayTrip);
        ImGui::Checkbox("Apollo-style", &mission.apolloStyle);
        ImGui::EndDisabled();

        if (ImGui::Button("Next: Edit Mission Sequence")) {
            updateMissionSequence();
            input_phase = InputPhase::Sequence;
        }

        ImGui::End();
        return false;
    }
    else if (input_phase == InputPhase::Sequence) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Mission Sequence Preview", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove);

        int i = 1;
        for (auto& step : msequence) {
            ImGui::PushID(i);
            switch (step.type) {
                case MissionPhaseType::TAKEOFF:
                    ImGui::BulletText("%i. Takeoff from %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::CIRCULARIZE: 
                    ImGui::BulletText("%i. Circularize orbit at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::HOHMANN_TRANSFER: 
                    ImGui::BulletText("%i Transfer %s --> %s", i, step.refBody->name, step.refBody2->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ATMOSPHERIC_BREAKING: 
                    ImGui::BulletText("%i. Atmospheric breaking at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::LANDING_PARACHUTES: 
                    ImGui::BulletText("%i. Landing at %s with parachutes", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::LANDING: 
                    ImGui::BulletText("%i. Landing at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::MINING: 
                    ImGui::BulletText("%i. Mining fuel at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ORBITAL_REFUELING: 
                    ImGui::BulletText("%i. Orbital refuelling %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
            }
            ImGui::PopID();
            ++i;
            ImGui::Separator();
            ImGui::Unindent();
        }

        ImGui::End();
    }

    return false;
}

void WindowMission::updateMissionSequence() {
    msequence.clear();

    auto fromTo = [this](const Body* from, const Body* to) {
        float startOrbit = from->atmHeight_km > 0 ? from->atmHeight_km + 10.0 : 30;
        float destOrbit  = to->atmHeight_km > 0 ? to->atmHeight_km + 10.0 : 30;

        msequence.push_back(MissionPhase { MissionPhaseType::TAKEOFF, from, nullptr, 0 });
        msequence.push_back(MissionPhase { MissionPhaseType::CIRCULARIZE, from, nullptr, startOrbit });
        if (from == to) {
            if (to->atmHeight_km > 0) {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, from, nullptr, startOrbit });
            }
            else {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING, from, nullptr, destOrbit });
            }
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::HOHMANN_TRANSFER, from, to, 0 });
        }
        if (to->atmHeight_km > 0) {
            msequence.push_back(MissionPhase { MissionPhaseType::ATMOSPHERIC_BREAKING, to, nullptr, destOrbit });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, to, nullptr, destOrbit });
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::CIRCULARIZE, to, nullptr, destOrbit });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING, to, nullptr, destOrbit });
        }
    };
    fromTo(mission.originBody, mission.destinationBody);
    if (!mission.oneWayTrip) {
        fromTo(mission.destinationBody, mission.originBody);
    }
}


