#include "WindowMission.h"
#include "imgui.h"


void WindowMission::render() {
    float mbh = ImGui::GetFrameHeight();
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight - mbh));
    ImGui::SetNextWindowPos(ImVec2(0, mbh));

    ImGui::Begin("Configure Mission", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    ImGui::End();
}

void WindowMission::onWindowResized(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

