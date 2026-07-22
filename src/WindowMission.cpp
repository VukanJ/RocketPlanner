#include "WindowMission.h"
#include "Orbit.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"
#include "implot.h"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>

#include "eigen3/Eigen/Geometry"
#include "DeltaV.h"
#include "helper.h"

Date::Date(int64_t seconds) {
    // Convert seconds to KSP date
    int64_t totalMinutes = seconds / 60;
    minute = totalMinutes % 60;
    int64_t totalHours = totalMinutes / 60;
    hour = totalHours % 6;
    int64_t totalDays = totalHours / 6;
    day = (totalDays % 426) + 1;
    year = (totalDays / 426) + 1;
}

int64_t Date::toSeconds() const {
    return static_cast<int64_t>(year - 1) * 426 * 6 * 60 * 60 +
           static_cast<int64_t>(day - 1) * 6 * 60 * 60 +
           static_cast<int64_t>(hour) * 60 * 60 +
           static_cast<int64_t>(minute) * 60;
}

namespace {
    constexpr int64_t kKerbalDaySeconds = 6 * 60 * 60;

    struct ColormapOption {
        const char* name;
        ImPlotColormap value;
    };

    constexpr int kNumPorkchopColormaps = 8;

    ImVec2 add(const ImVec2& lhs, const ImVec2& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    ImVec2 subtract(const ImVec2& lhs, const ImVec2& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    ColormapOption* getPorkchopColormaps() {
        static ColormapOption colormaps[] = {
            { "CMRmap", ImPlot::AddColormap("CMRmap", cmap_data_CMRmap, 256) },
            { "Viridis", ImPlotColormap_Viridis },
            { "Plasma", ImPlotColormap_Plasma },
            { "Hot", ImPlotColormap_Hot },
            { "Cool", ImPlotColormap_Cool },
            { "Jet", ImPlotColormap_Jet },
            { "Spectral", ImPlotColormap_Spectral },
            { "Greys", ImPlotColormap_Greys },
        };
        return colormaps;
    }

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

    void renderDateInput(const char* label, Date& date) {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputInt("Year", &date.year);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputInt("Day", &date.day);
        date.year = std::max(date.year, 1);
        date.day = std::clamp(date.day, 1, 426);
        date.hour = 0;
        date.minute = 0;
        ImGui::PopID();
    }
}

WindowMission::WindowMission() {
    porkchopPlot.launchStart = currentDate;
    porkchopPlot.launchEnd = Date(default_transfer_window_estimate(mission.originBody, mission.destinationBody));
    porkchopPlot.flightTimeStartDays = default_transfer_time_estimate(mission.originBody, mission.destinationBody) / kKerbalDaySeconds * 0.5f;
    porkchopPlot.flightTimeEndDays = porkchopPlot.flightTimeStartDays * 4.0f;
}

bool WindowMission::render() {
    bool endWin = false;
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
        return endWin;
    }
    else if (input_phase == InputPhase::Sequence) {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float panelWidth = 420.0f;
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, displaySize.y), ImGuiCond_Always);
        ImGui::Begin("Mission Sequence", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        ImGui::TextColored({1, 1, 0, 1}, "Ctrl+LMB to enter value");

        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.15f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.4f, 0.2f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.2f, 0.0f, 1.0f));
        if (ImGui::Checkbox("Advanced navigation", &advanced)) {
            updateMissionSequence();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Assume interplanetary burns performed in low orbit. Saves Δv via Oberth effect. Launch date optimization");
        }
        ImGui::PopStyleColor(4);

        if (advanced) {
            if (ImGui::Button("Auto-optimize transfer")) {
            }
            ImGui::SameLine();
            if (ImGui::Button("Porkchop plot")) {
                porkchopPlot.launchStart = currentDate;
                porkchopPlot.launchEnd = Date(default_transfer_window_estimate(mission.originBody, mission.destinationBody));
                float trans = default_transfer_time_estimate(mission.originBody, mission.destinationBody) / kKerbalDaySeconds;
                porkchopPlot.flightTimeStartDays = trans * 0.5f;
                porkchopPlot.flightTimeEndDays = porkchopPlot.flightTimeStartDays * 2.0f;
                porkchopPlot.isOpen = true;
            }
        }

        float dvTotal_min = 0;
        float dvTotal_max = 0;
        int updateCostFrom = -1;

        for (int i = 0; i < msequence.size(); ++i) {
            ImGui::PushID(i + 1);
            auto& step = msequence.at(i);
            if (step.dv_range.has_value()) {
                dvTotal_min += step.dv_range.min;
                dvTotal_max += step.dv_range.max;
            }
            switch (step.type) {
                case MissionPhaseType::TAKEOFF:
                    ImGui::Text("%i. Takeoff from %s", i, step.refBody->name);
                    ImGui::Indent();
                    if (ImGui::SliderFloat("##orbit", &step.alt1, step.refBody->atmHeight_km, 300, "Orbit: %.2f km", ImGuiSliderFlags_Logarithmic)) {
                        step.updateDeltaV();
                        updateCostFrom = i;
                    }
                    break;
                case MissionPhaseType::CIRCULARIZE_HYPERBOLIC:
                    ImGui::Text("%i. Circularize hyperbolic orbit around %s", i, step.refBody2->name);
                    ImGui::Indent();
                    if (ImGui::SliderFloat("##orbit2", &step.alt2, step.refBody2->atmHeight_km, 300, "Target Orbit: %.2f km", ImGuiSliderFlags_Logarithmic)) {
                        step.updateDeltaV();
                        updateCostFrom = i;
                    }
                    break;
                case MissionPhaseType::HOHMANN_TRANSFER: 
                    {
                        ImGui::Text("%i Transfer %s --> %s", i, step.refBody->name, step.refBody2->name);
                        ImGui::Indent();
                        if (step.dv_range.has_value() && step.dv_range.min == 0) {
                            ImGui::TextColored({0.5, 0.5, 0.5, 1}, "Δv: Set up by escape burn");
                        }
                    }
                    break;
                case MissionPhaseType::INCLINATION_CORRECTION: 
                    {
                        ImGui::Text("%i Inclination correction for %s", i, step.refBody2->name);
                        ImGui::Indent();
                    }
                    break;
                case MissionPhaseType::ATMOSPHERIC_BREAKING: 
                    ImGui::Text("%i. Atmospheric reentry at %s", i, step.refBody->name);
                    ImGui::Indent();
                    if (ImGui::SliderFloat("##reentryPE", &step.alt1, 0, step.refBody->atmHeight_km, "Reentry PE: %.1f km")) {
                        step.updateDeltaV();
                        for (int k = i - 1; k >= 0; --k) {
                            if (msequence[k].type == MissionPhaseType::ESCAPE) {
                                msequence[k].alt2 = step.alt1;
                                msequence[k].updateDeltaV();
                                break;
                            }
                        }
                        updateCostFrom = i;
                    }
                    break;
                case MissionPhaseType::LANDING_PARACHUTES: 
                    ImGui::Text("%i. Landing on %s with parachutes", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::LANDING: 
                    ImGui::Text("%i. Landing on %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ESCAPE: 
                    ImGui::Text("%i. Escape from %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ORBITAL_INSERTION: 
                    ImGui::Text("%i. Direct orbital transfer to %s", i, step.refBody2->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::MINING: 
                    ImGui::Text("%i. Mining fuel at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ORBITAL_REFUELING: 
                    ImGui::Text("%i. Orbital refuelling %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
            }
            if (step.dv_range.min > 0) {
                if (step.dv_range.single()) {
                    ImGui::Text("Δv: %.0f m/s", step.dv_range.min);
                }
                else {
                    ImGui::Text("Δv: %.0f - %.0f m/s", step.dv_range.min, step.dv_range.max);
                }
            }
            ImGui::PopID();
            ImGui::Separator();
            ImGui::Unindent();
        }
        if (std::abs(dvTotal_min - dvTotal_max) > 10) {
            ImGui::TextColored({0.5, 0.5, 1, 1}, "Total Δv: %.2f - %.2f m/s", dvTotal_min, dvTotal_max);
        }
        else {
            ImGui::TextColored({0.5, 0.5, 1, 1}, "Total Δv: %.2f m/s", 0.5f * (dvTotal_min + dvTotal_max));
        }
        if (ImGui::Button("Go Back")) { 
            endWin = false;
            input_phase = InputPhase::FromTo;
        }
        ImGui::SameLine();
        ImGui::SameLine();
        if (ImGui::Button("Accept")) { 
            endWin = true;
        }

        if (updateCostFrom > -1) {
            for (int j = updateCostFrom + 1; j < msequence.size(); ++j) {
                auto& step = msequence[j];
                auto& prev = msequence[j - 1];

                if (step.refBody == prev.refBody) {
                    step.alt1 = prev.alt1;
                }

                if (step.refBody == prev.refBody2) {
                    step.alt1 = prev.alt2;
                }

                step.updateDeltaV();
                if (step.type == MissionPhaseType::LANDING || step.type == MissionPhaseType::LANDING_PARACHUTES) {
                    // No more propagation
                    break;
                }
            }
        }

        ImGui::End();
    }

    systemMap.render(mission, currentDate.toSeconds());

    if (advanced) {
        update_solver = renderTimeInput();
        if (update_solver) {
            if (mission.originBody->orbit.AP > mission.destinationBody->orbit.AP) {
                showTransferOrbit(currentDate.toSeconds(), 0.505f * Orbit::elliptic(
                    mission.originBody->orbit.parent, mission.originBody->orbit.AP,
                    mission.destinationBody->orbit.PE).period());
            }
            else {
                showTransferOrbit(currentDate.toSeconds(), 0.505f * Orbit::elliptic(
                    mission.originBody->orbit.parent, mission.destinationBody->orbit.PE,
                    mission.originBody->orbit.AP).period());
            }
        }
    }

    if (porkchopPlot.isOpen) {
        renderPorkchopPlot();
    }

    return endWin;
}

void WindowMission::renderPorkchopPlot() {
    ImGui::SetNextWindowSize(ImVec2(900, 700), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Porkchop Plot", &porkchopPlot.isOpen)) {
        ImGui::End();
        return;
    }

    renderDateInput("Launch start", porkchopPlot.launchStart);
    renderDateInput("Launch end", porkchopPlot.launchEnd);
    ImGui::PushItemWidth(500.0f);
    ImGui::InputInt("Minimum flight time (days)", &porkchopPlot.flightTimeStartDays);
    ImGui::InputInt("Maximum flight time (days)", &porkchopPlot.flightTimeEndDays);
    ImGui::SliderInt("Resolution", &porkchopPlot.resolution, PorkchopPlot::minResolution, PorkchopPlot::maxResolution);
    ImGui::Combo("Colormap", &porkchopPlot.colormapIndex,
                 [](void* data, int index, const char** outText) {
                     const auto* colormaps = static_cast<const ColormapOption*>(data);
                     *outText = colormaps[index].name;
                     return true;
                 },
                  getPorkchopColormaps(),
                  kNumPorkchopColormaps);
    ImGui::PopItemWidth();

    porkchopPlot.flightTimeStartDays = std::max(porkchopPlot.flightTimeStartDays, 1);
    porkchopPlot.flightTimeEndDays = std::max(porkchopPlot.flightTimeEndDays, porkchopPlot.flightTimeStartDays + 1);

    if (ImGui::Button("Enter")) {
        generatePorkchopPlot();
    }

    if (porkchopPlot.calculated) {
        const double launchStartDay = static_cast<double>(porkchopPlot.calculatedLaunchStart.toSeconds()) / kKerbalDaySeconds;
        const double launchEndDay = static_cast<double>(porkchopPlot.calculatedLaunchEnd.toSeconds()) / kKerbalDaySeconds;

        ImGui::Separator();
        ImGui::Text("Total heliocentric transfer delta-v (m/s)");
        ImGui::SetNextItemWidth(250.0f);
        ImGui::SliderFloat("Color maximum", &porkchopPlot.colorMaxDeltaV, porkchopPlot.minDeltaV, porkchopPlot.maxDeltaV, "%.0f m/s");
        porkchopPlot.colorMaxDeltaV = std::max(porkchopPlot.colorMaxDeltaV, porkchopPlot.minDeltaV);

        const ImPlotColormap colormap = getPorkchopColormaps()[porkchopPlot.colormapIndex].value;
        ImPlot::PushColormap(colormap);
        constexpr float colorbarWidth = 100.0f;
        if (ImPlot::BeginPlot("##PorkchopHeatmap",
                              ImVec2(-colorbarWidth - ImGui::GetStyle().ItemSpacing.x, -1),
                              ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxes("Launch day (since Y1 D1)", "Flight time (days)");
            ImPlot::SetupAxisLimits(ImAxis_X1, launchStartDay, launchEndDay, ImPlotCond_Always);
            ImPlot::SetupAxisLimits( ImAxis_Y1, porkchopPlot.calculatedFlightTimeStartDays, porkchopPlot.calculatedFlightTimeEndDays, ImPlotCond_Always);
            ImPlot::PlotHeatmap("Total delta-v", porkchopPlot.deltaV.data(),
                                porkchopPlot.calculatedResolution,
                                porkchopPlot.calculatedResolution,
                                porkchopPlot.colorMaxDeltaV,
                                porkchopPlot.minDeltaV, nullptr,
                                ImPlotPoint(launchStartDay, porkchopPlot.calculatedFlightTimeStartDays),
                                ImPlotPoint(launchEndDay, porkchopPlot.calculatedFlightTimeEndDays));
            if (porkchopPlot.cheapestLaunchIndex >= 0 && porkchopPlot.cheapestFlightIndex >= 0) {
                const float launchFraction = static_cast<float>(porkchopPlot.cheapestLaunchIndex) /
                    (porkchopPlot.calculatedResolution - 1);
                const float flightFraction = static_cast<float>(porkchopPlot.cheapestFlightIndex) /
                    (porkchopPlot.calculatedResolution - 1);
                const double cheapestLaunchDay = launchStartDay + launchFraction * (launchEndDay - launchStartDay);
                const double cheapestFlightDays = porkchopPlot.calculatedFlightTimeStartDays +
                    flightFraction * (porkchopPlot.calculatedFlightTimeEndDays - porkchopPlot.calculatedFlightTimeStartDays);

                ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 10.0f, ImVec4(1, 1, 1, 1),
                                           2.0f, ImVec4(0, 0, 0, 1));
                ImPlot::PlotScatter("Cheapest transfer", &cheapestLaunchDay, &cheapestFlightDays, 1);

                char label[32];
                std::snprintf(label, sizeof(label), "%.0f m/s", porkchopPlot.minDeltaV);
                const ImVec2 labelPosition =
                    add(ImPlot::PlotToPixels(cheapestLaunchDay, cheapestFlightDays), ImVec2(8, -18));
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                for (const ImVec2 offset : {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)}) {
                    drawList->AddText(add(labelPosition, offset), IM_COL32(0, 0, 0, 255), label);
                }
                drawList->AddText(labelPosition, IM_COL32(255, 255, 255, 255), label);
            }

            if (ImPlot::IsPlotHovered()) {
                const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                const float launchFraction = std::clamp(
                    static_cast<float>((mouse.x - launchStartDay) / (launchEndDay - launchStartDay)),
                    0.0f, 1.0f);
                const float flightFraction = std::clamp(
                    static_cast<float>((mouse.y - porkchopPlot.calculatedFlightTimeStartDays) /
                                       (porkchopPlot.calculatedFlightTimeEndDays - porkchopPlot.calculatedFlightTimeStartDays)),
                    0.0f, 1.0f);
                const int launchIndex = std::lround(launchFraction * (porkchopPlot.calculatedResolution - 1));
                const int flightIndex = std::lround(flightFraction * (porkchopPlot.calculatedResolution - 1));
                const int plotRow = porkchopPlot.calculatedResolution - 1 - flightIndex;
                const float deltaV = porkchopPlot.deltaV[plotRow * porkchopPlot.calculatedResolution + launchIndex];

                char hoverText[96];
                std::snprintf(hoverText, sizeof(hoverText), "Launch: %.1f\nFlight: %.1f d\nDelta-v: %.0f m/s",
                              mouse.x, mouse.y, deltaV);
                const ImVec2 textSize = ImGui::CalcTextSize(hoverText);
                const ImVec2 textPosition = subtract(
                    subtract(add(ImPlot::GetPlotPos(), ImPlot::GetPlotSize()), textSize), ImVec2(8, 8));
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(subtract(textPosition, ImVec2(4, 4)),
                                        add(add(textPosition, textSize), ImVec2(4, 4)),
                                        IM_COL32(0, 0, 0, 192));
                drawList->AddText(textPosition, IM_COL32(255, 255, 255, 255), hoverText);

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const float selectedLaunchFraction = static_cast<float>(launchIndex) /
                        (porkchopPlot.calculatedResolution - 1);
                    const float selectedFlightFraction = static_cast<float>(flightIndex) /
                        (porkchopPlot.calculatedResolution - 1);
                    const float selectedLaunchSeconds = static_cast<float>(
                        porkchopPlot.calculatedLaunchStart.toSeconds()) +
                        selectedLaunchFraction * (porkchopPlot.calculatedLaunchEnd.toSeconds() -
                                                  porkchopPlot.calculatedLaunchStart.toSeconds());
                    const float selectedFlightDays = porkchopPlot.calculatedFlightTimeStartDays +
                        selectedFlightFraction * (porkchopPlot.calculatedFlightTimeEndDays -
                                                  porkchopPlot.calculatedFlightTimeStartDays);
                    currentDate = Date(std::llround(selectedLaunchSeconds));
                    showTransferOrbit(selectedLaunchSeconds, selectedFlightDays * kKerbalDaySeconds);
                }
            }
            ImPlot::EndPlot();
        }
        ImGui::SameLine();
        ImPlot::ColormapScale("Delta-v", porkchopPlot.colorMaxDeltaV, porkchopPlot.minDeltaV,
                              ImVec2(colorbarWidth, -1), "%.0f m/s");
        ImPlot::PopColormap();
    }

    ImGui::End();
}

void WindowMission::showTransferOrbit(float launchSeconds, float flightSeconds) {
    transfer_solver.init(mission.originBody->orbit.parent, mission.originBody->orbit,
                         mission.destinationBody->orbit);
    if (!transfer_solver.solve(launchSeconds, flightSeconds)) {
        systemMap.clearDebugOrbits();
        return;
    }

    auto nu1_1 = transfer_solver.transferOrbit1.trueAnomalyAt(transfer_solver.r1);
    auto nu2_1 = transfer_solver.transferOrbit1.trueAnomalyAt(transfer_solver.r2);
    if (nu2_1 < nu1_1) { nu2_1 += 2.0f * M_PI; }

    auto nu1_2 = transfer_solver.transferOrbit2.trueAnomalyAt(transfer_solver.r1);
    auto nu2_2 = transfer_solver.transferOrbit2.trueAnomalyAt(transfer_solver.r2);
    if (nu2_2 < nu1_2) { nu2_2 += 2.0f * M_PI; }

    systemMap.clearDebugOrbits();
    systemMap.addDebugOrbit({transfer_solver.transferOrbit1, IM_COL32(0, 200, 255, 200), 2.5f, nu1_1, nu2_1});
    systemMap.addDebugOrbit({transfer_solver.transferOrbit2, IM_COL32(255, 100, 200, 200), 2.5f, nu1_2, nu2_2});
}

void WindowMission::generatePorkchopPlot() {
    const int64_t launchStart = porkchopPlot.launchStart.toSeconds();
    const int64_t launchEnd = porkchopPlot.launchEnd.toSeconds();
    if (launchEnd <= launchStart) {
        porkchopPlot.calculated = false;
        return;
    }

    porkchopPlot.deltaV.assign(porkchopPlot.resolution * porkchopPlot.resolution,
                               std::numeric_limits<float>::infinity());
    porkchopPlot.minDeltaV = std::numeric_limits<float>::infinity();
    porkchopPlot.maxDeltaV = 0.0f;
    porkchopPlot.cheapestLaunchIndex = -1;
    porkchopPlot.cheapestFlightIndex = -1;

    LambertSolver solver;
    solver.init(mission.originBody->orbit.parent, mission.originBody->orbit,
                mission.destinationBody->orbit);
    for (int flightIndex = 0; flightIndex < porkchopPlot.resolution; ++flightIndex) {
        const float flightFraction = static_cast<float>(flightIndex) / (porkchopPlot.resolution - 1);
        const float flightDays = porkchopPlot.flightTimeStartDays + flightFraction * (porkchopPlot.flightTimeEndDays - porkchopPlot.flightTimeStartDays);
        const float flightSeconds = flightDays * kKerbalDaySeconds;

        for (int launchIndex = 0; launchIndex < porkchopPlot.resolution; ++launchIndex) {
            const float launchFraction =
                static_cast<float>(launchIndex) / (porkchopPlot.resolution - 1);
            const float launchSeconds = launchStart + launchFraction * (launchEnd - launchStart);
            if (!solver.solve(launchSeconds, flightSeconds)) {
                continue;
            }

            const float deltaV = 1000.0f * std::min(
                solver.deltaV1_depart + solver.deltaV1_arrive,
                solver.deltaV2_depart + solver.deltaV2_arrive);
            // ImPlot draws row zero at the top while its Y axis increases upward.
            const int plotRow = porkchopPlot.resolution - 1 - flightIndex;
            porkchopPlot.deltaV[plotRow * porkchopPlot.resolution + launchIndex] = deltaV;
            if (deltaV < porkchopPlot.minDeltaV) {
                porkchopPlot.minDeltaV = deltaV;
                porkchopPlot.cheapestLaunchIndex = launchIndex;
                porkchopPlot.cheapestFlightIndex = flightIndex;
            }
            porkchopPlot.maxDeltaV = std::max(porkchopPlot.maxDeltaV, deltaV);
        }
    }

    if (!std::isfinite(porkchopPlot.minDeltaV)) {
        porkchopPlot.calculated = false;
        return;
    }
    for (float& deltaV : porkchopPlot.deltaV) {
        if (!std::isfinite(deltaV)) {
            deltaV = porkchopPlot.maxDeltaV;
        }
    }
    porkchopPlot.calculatedResolution = porkchopPlot.resolution;
    porkchopPlot.calculatedLaunchStart = porkchopPlot.launchStart;
    porkchopPlot.calculatedLaunchEnd = porkchopPlot.launchEnd;
    porkchopPlot.calculatedFlightTimeStartDays = porkchopPlot.flightTimeStartDays;
    porkchopPlot.calculatedFlightTimeEndDays = porkchopPlot.flightTimeEndDays;
    porkchopPlot.colorMaxDeltaV = (porkchopPlot.maxDeltaV - porkchopPlot.minDeltaV) * 0.2f + porkchopPlot.minDeltaV;
    porkchopPlot.calculated = true;
}

bool WindowMission::renderTimeInput() {
    bool update = false;
    ImGui::SetNextWindowPos(ImVec2(450, 30), ImGuiCond_Appearing);
    ImGui::Begin("Mission Time", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PushItemWidth(110);
    if (ImGui::InputInt("Year", &currentDate.year)) {
        if (currentDate.year < 1) { currentDate.year = 1; }
        update = true;
    }
    if (ImGui::InputInt("Day", &currentDate.day)) {
        if (currentDate.day < 1) { currentDate.day = 426; }
        if (currentDate.day > 426) { currentDate.day = 1; }
        update = true;
    }
    if (ImGui::InputInt("Hour", &currentDate.hour)) {
        if (currentDate.hour < 0) { currentDate.hour = 5; }
        if (currentDate.hour > 5) { currentDate.hour = 0; }
        update = true;
    }
    if (ImGui::InputInt("Minute", &currentDate.minute)) {
        if (currentDate.minute < 0) { currentDate.minute = 59; }
        if (currentDate.minute > 59) { currentDate.minute = 0; }
        update = true;
    }

    ImGui::PopItemWidth();
    ImGui::End();
    return update;
}

void WindowMission::updateMissionSequence() {
    msequence.clear();

    auto fromTo = [this](const Body* from, const Body* to) {
        float startOrbit = from->atmHeight_km > 0 ? from->atmHeight_km + 10.0 : 30;
        float destOrbit  = to->atmHeight_km > 0 ? to->atmHeight_km + 10.0 : 30;
        float reentryPE  = destOrbit;

        if (to == from->orbit.parent) {
            reentryPE = 30;  // guarantee atmospheric capture
        }

        msequence.push_back(MissionPhase::takeoff(from, startOrbit));

        if (from == to) {
            if (to->atmHeight_km > 0) {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, from, nullptr, startOrbit });
            }
            else {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING, from, nullptr, destOrbit });
            }
        }
        else {
            if (to->orbit.parent == from->orbit.parent) {
                // Planet to planet — both orbit the same parent body
                if (advanced) {
                    msequence.push_back(MissionPhase { MissionPhaseType::ORBITAL_INSERTION, from, to, startOrbit, destOrbit });
                }
                else {
                    msequence.push_back(MissionPhase { MissionPhaseType::ESCAPE, from, to, startOrbit, destOrbit });
                    if (to->orbit.inclination != from->orbit.inclination) {
                        msequence.push_back(MissionPhase { MissionPhaseType::INCLINATION_CORRECTION, from, to, startOrbit });
                    }
                }
            }
            if (to == from->orbit.parent) {
                // Return from natural satellite. Need to escape first
                msequence.push_back(MissionPhase { MissionPhaseType::ESCAPE, from, to, startOrbit, reentryPE });
            }
            if (to->orbit.parent == from) {
                // Parent to moon — inclination correction w.r.t. parent's equator
                if (to->orbit.inclination != 0) { // origin orbit is in parents ecliptic
                    msequence.push_back(MissionPhase { MissionPhaseType::INCLINATION_CORRECTION, from, to, startOrbit });
                }
            }
            if (!advanced) {
                msequence.push_back(MissionPhase::hohmann(from, to, startOrbit, destOrbit));
            }
        }
        if (to->atmHeight_km > 0) {
            msequence.push_back(MissionPhase { MissionPhaseType::ATMOSPHERIC_BREAKING, to, nullptr, reentryPE });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, to, nullptr, destOrbit });
        }
        else {
            msequence.push_back(MissionPhase::circularize_hyperbolic(from, to, startOrbit, destOrbit));
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING, to, nullptr, destOrbit });
        }
    };
    fromTo(mission.originBody, mission.destinationBody);
    if (!mission.oneWayTrip) {
        fromTo(mission.destinationBody, mission.originBody);
    }

    for (auto& step : msequence) {
        step.updateDeltaV();
    }
}

void MissionPhase::updateDeltaV() {
    switch (type) {
        case MissionPhaseType::LANDING: 
            dv_range = naiveTakeoffLandingCost(refBody, alt1); 
            break;
        case MissionPhaseType::TAKEOFF: 
            dv_range = naiveTakeoffLandingCost(refBody, alt1); 
            break;
        case MissionPhaseType::ESCAPE:
            if (refBody->orbit.parent == refBody2->orbit.parent) {
                // Planet to planet, or Moon to Moon
                // No direct maneuver, since inclination correction might be needed.
                dv_range = escapeBurnCost(refBody, alt1);
            }
            else {
                // Combine Escape and Hohmann return into a single maneuver
                // Cost should be the same as for circularizing hyperbolic orbit 
                // after a Hohmann transfer, just in reverse. (Time-reversal symmetry)
                dv_range = circularizeHyperbolicCost(refBody->orbit.parent, refBody, alt2, alt1, true);
            }
            break;
        case MissionPhaseType::CIRCULARIZE_HYPERBOLIC:
            if (refBody2 && refBody2 == refBody->orbit.parent) {
                dv_range = circularizeHyperbolicCost(refBody2, refBody, alt2, alt1, true);
            } else if (refBody2 && refBody2->orbit.parent == refBody->orbit.parent) {
                // Planet -> planet
                dv_range = circularizeHyperbolicCost(refBody, refBody2, alt1, alt2, true);
            } else {
                dv_range = circularizeHyperbolicCost(refBody, refBody2, alt1, alt2, true);
            }
            break;
        case MissionPhaseType::HOHMANN_TRANSFER:
            dv_range = hohmannTransferCost(refBody, refBody2, alt1);
            break;
        case MissionPhaseType::ORBITAL_INSERTION:
            dv_range = orbitalInsertion(refBody, refBody2, alt1);
            break;
        case MissionPhaseType::INCLINATION_CORRECTION:
            if (refBody2 && refBody2->orbit.parent == refBody) {
                // Planet → moon: ship's parking orbit vs moon's orbit around planet
                Orbit shipOrbit = Orbit::circular(refBody, alt1 + refBody->radius_km);
                dv_range = inclinationCorrectionCost(shipOrbit, refBody2->orbit);
            } else {
                // Planet → planet: orbits around shared parent
                dv_range = inclinationCorrectionCost(refBody->orbit, refBody2->orbit);
            }
            break;
        default: dv_range = 0;
    }
}
