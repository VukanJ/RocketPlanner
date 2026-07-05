#include "WindowMission.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"

namespace {
    void renderBodyCombo(const char* label, const Body*& selected) {
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
    }
}

bool WindowMission::render() {
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

    bool next = false;
    if (ImGui::Button("Design Rocket")) {
        next = true;
    }

    ImGui::End();

    return next;
}


