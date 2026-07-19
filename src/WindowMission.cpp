#include "WindowMission.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "eigen3/Eigen/Geometry"
#include "DeltaV.h"

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

        if (ImGui::Button("Calculate launch window")) {
            transfer_solver.init(mission.originBody->orbit.parent, mission.originBody->orbit, mission.destinationBody->orbit, currentDate.toSeconds());
            transfer_solver.solve(220*6*60*60, 270*6*60*60); // Duna transfer after t=0

            auto nu1_1 = transfer_solver.transferOrbit1.trueAnomalyAt(transfer_solver.r1);
            auto nu2_1 = transfer_solver.transferOrbit1.trueAnomalyAt(transfer_solver.r2);
            if (nu2_1 < nu1_1) nu2_1 += 2.0f * M_PI;

            auto nu1_2 = transfer_solver.transferOrbit2.trueAnomalyAt(transfer_solver.r1);
            auto nu2_2 = transfer_solver.transferOrbit2.trueAnomalyAt(transfer_solver.r2);
            if (nu2_2 < nu1_2) nu2_2 += 2.0f * M_PI;

            systemMap.clearDebugOrbits();
            systemMap.addDebugOrbit({transfer_solver.transferOrbit1, IM_COL32(0, 200, 255, 200), 2.5f, nu1_1, nu2_1});
            systemMap.addDebugOrbit({transfer_solver.transferOrbit2, IM_COL32(255, 100, 200, 200), 2.5f, nu1_2, nu2_2});
        }

        float dvTotal_min = 0;
        float dvTotal_max = 0;
        int updateCostFrom = -1;

        for (int i = 0; i < msequence.size(); ++i) {
            ImGui::PushID(i + 1);
            auto& step = msequence.at(i);
            ImGui::Checkbox("##TEST", &step.active); ImGui::SameLine();
            ImGui::BeginDisabled(!step.active);
            if (step.active && step.dv_range.has_value()) {
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
            ImGui::EndDisabled();
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
        renderTimeInput();
    }

    return endWin;
}

void WindowMission::renderTimeInput() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Appearing);
    ImGui::Begin("Mission Time", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PushItemWidth(110);
    ImGui::InputInt("Year", &currentDate.year);
    if (currentDate.year < 1) { currentDate.year = 1; }
    if (ImGui::InputInt("Day", &currentDate.day)) {
        if (currentDate.day < 1) { currentDate.day = 426; }
        if (currentDate.day > 426) { currentDate.day = 1; }
    }
    if (ImGui::InputInt("Hour", &currentDate.hour)) {
        if (currentDate.hour < 0) { currentDate.hour = 5; }
        if (currentDate.hour > 5) { currentDate.hour = 0; }
    }
    if (ImGui::InputInt("Minute", &currentDate.minute)) {
        if (currentDate.minute < 0) { currentDate.minute = 59; }
        if (currentDate.minute > 59) { currentDate.minute = 0; }
    }

    ImGui::PopItemWidth();
    ImGui::End();
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
