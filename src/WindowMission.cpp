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
#include <limits>

#include "eigen3/Eigen/Geometry"
#include "DeltaV.h"
#include "helper.h"

ThreadPool WindowMission::threadPool = ThreadPool(2);

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

    ImVec2 add(const ImVec2& lhs, const ImVec2& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    ImVec2 subtract(const ImVec2& lhs, const ImVec2& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    void renderInvertedColormapScale(const char* id, float width, float height,
                                     float logMax = 0.0f) {
        const ImVec2 position = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(width, height));

        constexpr int segments = 256;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (int segment = 0; segment < segments; ++segment) {
            const float left = static_cast<float>(segment) / segments;
            const float right = static_cast<float>(segment + 1) / segments;
            const float tLeft = logMax > 0.0f
                ? std::log1p(left * (std::expm1(logMax))) / logMax : left;
            const float tRight = logMax > 0.0f
                ? std::log1p(right * (std::expm1(logMax))) / logMax : right;
            const ImU32 leftColor = ImGui::ColorConvertFloat4ToU32(ImPlot::SampleColormap(1.0f - tLeft));
            const ImU32 rightColor = ImGui::ColorConvertFloat4ToU32(ImPlot::SampleColormap(1.0f - tRight));
            drawList->AddRectFilledMultiColor(
                ImVec2(position.x + width * left, position.y),
                ImVec2(position.x + width * right, position.y + height),
                leftColor, rightColor, rightColor, leftColor);
        }
        drawList->AddRect(position, ImVec2(position.x + width, position.y + height),
                          ImGui::GetColorU32(ImGuiCol_Border));
    }

    constexpr int kNumPorkchopColormaps = 8;
    ColormapOption* getPorkchopColormaps() {
        static ColormapOption colormaps[] = {
            { "CMRmap", ImPlot::AddColormap("CMRmap", cmap_data_CMRmap, 256) },
            { "Viridis", ImPlotColormap_Viridis },
            { "Plasma", ImPlotColormap_Plasma },
            { "Hot", ImPlotColormap_Hot },
            { "Jet", ImPlotColormap_Jet },
            { "Spectral", ImPlotColormap_Spectral },
            { "Greys", ImPlotColormap_Greys },
            { "Pink", ImPlotColormap_Pink },
        };
        return colormaps;
    }

    bool renderBodyCombo(const char* label, const Body*& selected, const Body* exclude) {
        bool changed = false;
        const char* preview = "None";
        for (auto& entry : bodyTable) {
            if (entry.body == selected && entry.body != &KspSystem::Kerbol && entry.body != exclude) {
                preview = entry.name;
                break;
            }
        }
        if (ImGui::BeginCombo(label, preview)) {
            for (auto& entry : bodyTable) {
                if (entry.body == &KspSystem::Kerbol || entry.body == exclude) { continue; }
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
        ImGui::PushItemWidth(150.0f);
        ImGui::InputInt("Year", &date.year);
        ImGui::SameLine();
        ImGui::InputInt("Day", &date.day);
        ImGui::PopItemWidth();
        date.year = std::max(date.year, 1);
        date.day = std::clamp(date.day, 1, 426);
        date.hour = 0;
        date.minute = 0;
        ImGui::PopID();
    }

    void renderDurationInput(const char* label, int& years, int& days) {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        ImGui::InputInt("Years", &years);
        ImGui::SameLine();
        ImGui::InputInt("Days", &days);
        ImGui::PopItemWidth();
        years = std::max(years, 0);
        days = std::max(days, 0);
        ImGui::PopID();
    }

    void updateSelectedTransferCosts(std::vector<MissionPhase>& sequence,
                                     const Mission& mission,
                                     const LambertSolver& solver) {
        const bool firstTransferIsCheaper =
            solver.deltaV1_depart + solver.deltaV1_arrive
            <= solver.deltaV2_depart + solver.deltaV2_arrive;
        const float departureCost = firstTransferIsCheaper ? solver.deltaV1_depart : solver.deltaV2_depart;
        const float arrivalCost = firstTransferIsCheaper ? solver.deltaV1_arrive : solver.deltaV2_arrive;

        for (MissionPhase& phase : sequence) {
            if (phase.refBody != mission.originBody || phase.refBody2 != mission.destinationBody) {
                continue;
            }
            if (phase.type == MissionPhaseType::ORBITAL_INSERTION) {
                phase.dv_range = departureCost;
            }
            else if (phase.type == MissionPhaseType::CIRCULARIZE_HYPERBOLIC) {
                phase.dv_range = arrivalCost;
            }
        }
    }
}

WindowMission::WindowMission() { }

bool WindowMission::render() {
    bool endWin = false;
    if (input_phase == InputPhase::FromTo) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("Configure Mission", nullptr, ImGuiWindowFlags_AlwaysAutoResize 
                                                 | ImGuiWindowFlags_NoCollapse 
                                                 | ImGuiWindowFlags_NoBringToFrontOnFocus 
                                                 | ImGuiWindowFlags_NoMove);

        ImGui::SeparatorText("Origin");
        renderBodyCombo("##origin", mission.originBody, mission.destinationBody);

        ImGui::SeparatorText("Destination");

        renderBodyCombo("##dest", mission.destinationBody, mission.originBody);

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
        ImGui::Begin("Mission Sequence", nullptr, ImGuiWindowFlags_NoCollapse 
                                                | ImGuiWindowFlags_NoBringToFrontOnFocus 
                                                | ImGuiWindowFlags_NoMove 
                                                | ImGuiWindowFlags_AlwaysVerticalScrollbar);

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

                    if (advanced) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.0f));
                        if (ImGui::Button("Open Solver")) {
                            step.porkchopPlot->init(currentDate);
                            step.porkchopPlot->winOpen = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Auto-solve")) {
                        }
                        ImGui::PopStyleColor();
                    }
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

    for (auto& phase : msequence) {
        if (phase.porkchopPlot && phase.porkchopPlot->winOpen) {
            phase.porkchopPlot->render();
        }
    }

    return endWin;
}

void PorkchopPlot::render() {
    if (!winOpen) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(900, 700), ImGuiCond_FirstUseEver);
    ImGui::PushID(this);
    char label[64];
    std::snprintf(label, sizeof(label), "Transfer optimizer: %s ==> %s", phase->refBody->name, phase->refBody2->name);
    ImGui::Begin(label, &winOpen);

    if (progress >= 0) {
        ImGui::BeginDisabled();
    }
    renderDateInput("Launch start", launchStart);
    renderDurationInput("Launch window", launchWindowDurationYears, launchWindowDurationDays);
    updateLaunchEnd();
    ImGui::PushItemWidth(150.0f);
    ImGui::Text("Flight time"); ImGui::SameLine();
    ImGui::Text("Min:"); ImGui::SameLine(); ImGui::InputInt("##Min", &flightTimeStartDays); ImGui::SameLine(); 
    ImGui::Text("Max:"); ImGui::SameLine(); ImGui::InputInt("##Max", &flightTimeEndDays);

    ImGui::SliderInt("Resolution", &resolution, PorkchopPlot::minResolution, PorkchopPlot::maxResolution);
    ImGui::SameLine();
    if (ImGui::Button("Reset search window")) {
        init(this->launchStart);
    }
    ImGui::PopItemWidth();

    if (progress >= 0) {
        ImGui::EndDisabled();
    }

    flightTimeStartDays = std::max(flightTimeStartDays, 1);
    flightTimeEndDays = std::max(flightTimeEndDays, flightTimeStartDays + 1);

    if (progress < 0) { // Make sure button is not pressed while calculation is in progress
        const bool hasLaunchWindow =
            launchWindowDurationYears > 0 || launchWindowDurationDays > 0;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 1.0f, 1.0f));
        ImGui::BeginDisabled(!hasLaunchWindow);
        if (ImGui::Button("Generate", ImVec2(-1, 0))) {
            WindowMission::threadPool.send([this]() { generate(); });
        }
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        if (!hasLaunchWindow) {
            ImGui::TextDisabled("Set a launch window duration greater than zero to generate a plot.");
        }
    }

    if (calculated) {
        const double launchStartDay = static_cast<double>(calculatedLaunchStart.toSeconds()) / kKerbalDaySeconds;
        const double launchEndDay = static_cast<double>(calculatedLaunchEnd.toSeconds()) / kKerbalDaySeconds;

        ImGui::Separator();
        ImGui::Text("Total heliocentric transfer delta-v (m/s)");

        ImGui::SetNextItemWidth(250.0f);
        ImGui::Combo("Colormap", &colormapIndex,
                     [](void* data, int index, const char** outText) {
                         const auto* colormaps = static_cast<const ColormapOption*>(data);
                         *outText = colormaps[index].name;
                         return true;
                     },
                      getPorkchopColormaps(),
                      kNumPorkchopColormaps);

        ImGui::SetNextItemWidth(250.0f);
        ImGui::SliderFloat("Color maximum", &colorMaxDeltaV, minDeltaV, maxDeltaV,
                           "%.0f m/s", ImGuiSliderFlags_Logarithmic);
        colorMaxDeltaV = std::max(colorMaxDeltaV, minDeltaV);
        ImGui::SameLine();
        if (ImGui::Button(useLogColorScale ? "Log" : "Lin")) {
            useLogColorScale = !useLogColorScale;
        }

        const ImPlotColormap colormap = getPorkchopColormaps()[colormapIndex].value;
        ImPlot::PushColormap(colormap);
        ImGui::TextColored({1, 1, 0, 1}, "Select transfer with left mouse button");
        if (ImPlot::BeginPlot("##PorkchopHeatmap",
                              ImVec2(-1, -(ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y)),
                              ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxes("Launch day (since Y1 D1)", "Flight time (days)");
            ImPlot::SetupAxisLimits(ImAxis_X1, launchStartDay, launchEndDay, ImPlotCond_Always);
            ImPlot::SetupAxisLimits( ImAxis_Y1, calculatedFlightTimeStartDays, calculatedFlightTimeEndDays, ImPlotCond_Always);
            const bool logScale = useLogColorScale && colorMaxDeltaV > minDeltaV;
            const float scaleMin = logScale ? 0.0f : minDeltaV;
            const float scaleMax = logScale ? std::log1p(colorMaxDeltaV - minDeltaV) : colorMaxDeltaV;
            const float* dataPtr = logScale ? deltaVLog.data() : deltaV.data();
            ImPlot::PlotHeatmap("Total delta-v", dataPtr,
                                calculatedResolution,
                                calculatedResolution,
                                scaleMax,
                                scaleMin, nullptr,
                                ImPlotPoint(launchStartDay, calculatedFlightTimeStartDays),
                                ImPlotPoint(launchEndDay, calculatedFlightTimeEndDays));
            if (cheapestIndex >= 0) {
                const int cheapestLaunchIndex = cheapestIndex % calculatedResolution;
                const int cheapestFlightIndex =
                    calculatedResolution - 1 - cheapestIndex / calculatedResolution;
                const float launchFraction = static_cast<float>(cheapestLaunchIndex) /
                    (calculatedResolution - 1);
                const float flightFraction = static_cast<float>(cheapestFlightIndex) /
                    (calculatedResolution - 1);
                const double cheapestLaunchDay = launchStartDay + launchFraction * (launchEndDay - launchStartDay);
                const double cheapestFlightDays = calculatedFlightTimeStartDays +
                    flightFraction * (calculatedFlightTimeEndDays - calculatedFlightTimeStartDays);

                ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 10.0f, ImVec4(1, 1, 1, 1),
                                           2.0f, ImVec4(0, 0, 0, 1));
                ImPlot::PlotScatter("Cheapest transfer", &cheapestLaunchDay, &cheapestFlightDays, 1);

                char label[32];
                std::snprintf(label, sizeof(label), "%.0f m/s", minDeltaV);
                const ImVec2 labelPosition =
                    add(ImPlot::PlotToPixels(cheapestLaunchDay, cheapestFlightDays), ImVec2(8, -18));
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                for (const ImVec2 offset : {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)}) {
                    drawList->AddText(add(labelPosition, offset), IM_COL32(0, 0, 0, 255), label);
                }
                drawList->AddText(labelPosition, IM_COL32(255, 255, 255, 255), label);
            }

            if (selectedIndex >= 0) {
                const int selectedLaunchIndex = selectedIndex % calculatedResolution;
                const int selectedFlightIndex =
                    calculatedResolution - 1 - selectedIndex / calculatedResolution;
                const float launchFraction = static_cast<float>(selectedLaunchIndex) /
                    (calculatedResolution - 1);
                const float flightFraction = static_cast<float>(selectedFlightIndex) /
                    (calculatedResolution - 1);
                const double selectedLaunchDay =
                    launchStartDay + launchFraction * (launchEndDay - launchStartDay);
                const double selectedFlightDays = calculatedFlightTimeStartDays +
                    flightFraction * (calculatedFlightTimeEndDays - calculatedFlightTimeStartDays);
                const float selectedDeltaV = deltaV[selectedIndex];

                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 9.0f, ImVec4(1, 1, 0, 1),
                                           2.0f, ImVec4(0, 0, 0, 1));
                ImPlot::PlotScatter("Selected transfer", &selectedLaunchDay, &selectedFlightDays, 1);

                char selectedLabel[32];
                std::snprintf(selectedLabel, sizeof(selectedLabel), "%.0f m/s", selectedDeltaV);
                const ImVec2 labelPosition =
                    add(ImPlot::PlotToPixels(selectedLaunchDay, selectedFlightDays), ImVec2(8, 8));
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                for (const ImVec2 offset : {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)}) {
                    drawList->AddText(add(labelPosition, offset), IM_COL32(0, 0, 0, 255), selectedLabel);
                }
                drawList->AddText(labelPosition, IM_COL32(255, 255, 0, 255), selectedLabel);
            }

            if (ImPlot::IsPlotHovered()) {
                const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                const float launchFraction = std::clamp(
                    static_cast<float>((mouse.x - launchStartDay) / (launchEndDay - launchStartDay)),
                    0.0f, 1.0f);
                const float flightFraction = std::clamp(
                    static_cast<float>((mouse.y - calculatedFlightTimeStartDays) /
                                       (calculatedFlightTimeEndDays - calculatedFlightTimeStartDays)),
                    0.0f, 1.0f);
                const int launchIndex = std::lround(launchFraction * (calculatedResolution - 1));
                const int flightIndex = std::lround(flightFraction * (calculatedResolution - 1));
                const int plotRow = calculatedResolution - 1 - flightIndex;
                const float deltaV_pixel = deltaV[plotRow * calculatedResolution + launchIndex];

                char hoverText[96];
                std::snprintf(hoverText, sizeof(hoverText), "Launch: %.1f\nFlight: %.1f d\nDelta-v: %.0f m/s",
                              mouse.x, mouse.y, deltaV_pixel);
                const ImVec2 textSize = ImGui::CalcTextSize(hoverText);
                const ImVec2 textPosition = subtract(
                    subtract(add(ImPlot::GetPlotPos(), ImPlot::GetPlotSize()), textSize), ImVec2(8, 8));
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(subtract(textPosition, ImVec2(4, 4)),
                                        add(add(textPosition, textSize), ImVec2(4, 4)),
                                        IM_COL32(0, 0, 0, 192));
                drawList->AddText(textPosition, IM_COL32(255, 255, 255, 255), hoverText);

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selectedIndex = plotRow * calculatedResolution + launchIndex;
                    const float selectedLaunchFraction = static_cast<float>(launchIndex) /
                        (calculatedResolution - 1);
                    const float selectedFlightFraction = static_cast<float>(flightIndex) /
                        (calculatedResolution - 1);
                    const float selectedLaunchSeconds = static_cast<float>(
                        calculatedLaunchStart.toSeconds()) +
                        selectedLaunchFraction * (calculatedLaunchEnd.toSeconds() -
                                                  calculatedLaunchStart.toSeconds());
                    const float selectedFlightDays = calculatedFlightTimeStartDays +
                        selectedFlightFraction * (calculatedFlightTimeEndDays -
                                                  calculatedFlightTimeStartDays);
                    //currentDate = Date(std::llround(selectedLaunchSeconds));
                    //showTransferOrbit(selectedLaunchSeconds, selectedFlightDays * kKerbalDaySeconds);
                }
            }
            ImPlot::EndPlot();
        }

        char minLabel[32];
        char maxLabel[32];
        std::snprintf(minLabel, sizeof(minLabel), "%.0f m/s", minDeltaV);
        std::snprintf(maxLabel, sizeof(maxLabel), "%.0f m/s", colorMaxDeltaV);
        constexpr float scaleSpacing = 4.0f;
        const float scaleWidth = std::max(
            0.0f,
            ImGui::GetContentRegionAvail().x
                - ImGui::CalcTextSize(minLabel).x
                - ImGui::CalcTextSize(maxLabel).x
                - 2.0f * scaleSpacing);
        ImGui::TextUnformatted(minLabel);
        ImGui::SameLine(0.0f, scaleSpacing);
        const float logMax = (useLogColorScale && colorMaxDeltaV > minDeltaV)
            ? std::log1p(colorMaxDeltaV - minDeltaV) : 0.0f;
        renderInvertedColormapScale("##DeltaVScale", scaleWidth, ImGui::GetFrameHeight(), logMax);
        ImGui::SameLine(0.0f, scaleSpacing);
        ImGui::TextUnformatted(maxLabel);
        ImPlot::PopColormap();
    }
    else {
        if (progress > 0.0f) {
            ImGui::ProgressBar(progress, ImVec2(-1, 0));
        }
    }

    ImGui::End();
    ImGui::PopID();
}

void WindowMission::showTransferOrbit(float launchSeconds, float flightSeconds) {
    //const auto [startAltitude, targetAltitude] = transferOrbitAltitudes(mission, msequence);
    transfer_solver.init(mission.originBody->orbit.parent, 
                         mission.originBody->orbit,
                         mission.destinationBody->orbit, 
                         mission.originBody,
                         mission.destinationBody, 
                         30, // TODO FIX
                         30);// TODO FIX
    if (!transfer_solver.solve(launchSeconds, flightSeconds)) {
        systemMap.clearDebugOrbits();
        return;
    }
    updateSelectedTransferCosts(msequence, mission, transfer_solver);

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

void PorkchopPlot::init(Date launchDate) {
    launchStart = launchDate;
    const int windowDays = std::ceil(
        default_transfer_window_estimate(phase->refBody, phase->refBody2) / kKerbalDaySeconds);
    launchWindowDurationYears = windowDays / 426;
    launchWindowDurationDays = windowDays % 426;
    updateLaunchEnd();
    float trans = default_transfer_time_estimate(phase->refBody, phase->refBody2) / kKerbalDaySeconds;
    flightTimeStartDays = trans * 0.5f;
    flightTimeEndDays = flightTimeStartDays * 2.0f;
}

void PorkchopPlot::updateLaunchEnd() {
    const int64_t durationDays =
        static_cast<int64_t>(launchWindowDurationYears) * 426 + launchWindowDurationDays;
    launchEnd = Date(launchStart.toSeconds() + durationDays * kKerbalDaySeconds);
}

void PorkchopPlot::generate() {
    calculated = false;
    auto startSeconds = launchStart.toSeconds();
    auto endSeconds = launchEnd.toSeconds();

    if (endSeconds <= startSeconds) { return; }

    deltaV.assign(resolution * resolution, std::numeric_limits<float>::infinity());
    minDeltaV = std::numeric_limits<float>::infinity();
    maxDeltaV = 0.0f;
    cheapestIndex = -1;
    selectedIndex = -1;

    auto* origin = phase->refBody;
    auto* target = phase->refBody2;

    LambertSolver solver;
    solver.init(origin->orbit.parent, origin->orbit,
                target->orbit, origin,
                target, phase->alt1, phase->alt2);
    for (int flightIndex = 0; flightIndex < resolution; ++flightIndex) {
        const float flightFraction = static_cast<float>(flightIndex) / (resolution - 1);
        const float flightDays = flightTimeStartDays + flightFraction * (flightTimeEndDays - flightTimeStartDays);
        const float flightSeconds = flightDays * kKerbalDaySeconds;
        progress = static_cast<float>(flightIndex) / (resolution - 1);

        for (int launchIndex = 0; launchIndex < resolution; ++launchIndex) {
            const float launchFraction = static_cast<float>(launchIndex) / (resolution - 1);
            const float launchSeconds = startSeconds + launchFraction * (endSeconds - startSeconds);
            if (!solver.solve(launchSeconds, flightSeconds)) {
                continue;
            }

            const float min_dv = std::min(
                solver.deltaV1_depart + solver.deltaV1_arrive,
                solver.deltaV2_depart + solver.deltaV2_arrive);
            // ImPlot draws row zero at the top while its Y axis increases upward.
            const int plotRow = resolution - 1 - flightIndex;
            deltaV[plotRow * resolution + launchIndex] = min_dv;
            if (min_dv < minDeltaV) {
                minDeltaV = min_dv;
                cheapestIndex = plotRow * resolution + launchIndex;
            }
            maxDeltaV = std::max(maxDeltaV, min_dv);
        }
    }

    if (!std::isfinite(minDeltaV)) {
        calculated = false;
        return;
    }
    for (float& dv : deltaV) {
        if (!std::isfinite(dv)) {
            dv = maxDeltaV;
        }
    }
    deltaVLog.resize(deltaV.size());
    for (size_t i = 0; i < deltaV.size(); ++i) {
        deltaVLog[i] = std::log1p(deltaV[i] - minDeltaV);
    }
    calculatedResolution          = resolution;
    calculatedLaunchStart         = launchStart;
    calculatedLaunchEnd           = launchEnd;
    calculatedFlightTimeStartDays = flightTimeStartDays;
    calculatedFlightTimeEndDays   = flightTimeEndDays;
    colorMaxDeltaV                = (maxDeltaV - minDeltaV) * 0.2f + minDeltaV;
    calculated = true;
    progress = -1.0f; // Disable progress bar
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

    // Initialize porkchop plots
    for (auto& step : msequence) {
        if (step.type == MissionPhaseType::ORBITAL_INSERTION) {
            step.porkchopPlot = std::make_unique<PorkchopPlot>(&step);
        }
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
