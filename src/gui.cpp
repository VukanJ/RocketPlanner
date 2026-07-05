#include "gui.h"
#include <memory>

#include "imgui.h"

GUI::GUI(const PartInfoList& engines) { 
    winSim = std::make_unique<WindowSimulator>(engines);
    winM = std::make_unique<WindowMission>();
}

void GUI::render() {
    drawMenuBar();
    switch (phase) {
        case Phase::MISSION_SELECT:
            winM->render();
            break;
        case Phase::ROCKET:
            winM->render();
            break;
    }
    if (showDemo) {
        ImGui::ShowDemoWindow(&showDemo);
    }
}

void GUI::onWindowResized(int width, int height) {
    winSim->onWindowResized(width, height);
    winM->onWindowResized(width, height);
}

void GUI::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("ImGui Demo Window", nullptr, &showDemo);
            ImGui::EndMenu();
        }
        bool dummy;
        if (ImGui::BeginMenu("Appearance")) {
            if (ImGui::MenuItem("Dark", nullptr, &dummy)) {
                ImGui::StyleColorsDark();
            }
            if (ImGui::MenuItem("Light", nullptr, &dummy)) {
                ImGui::StyleColorsLight();
            }
            if (ImGui::MenuItem("Classic", nullptr, &dummy)) {
                ImGui::StyleColorsClassic();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

