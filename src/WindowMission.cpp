#include "WindowMission.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "eigen3/Eigen/Geometry"
#include "DeltaV.h"

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
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Mission Sequence", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove);

        ImGui::TextColored({1, 1, 0, 1}, "Ctrl+LMB to enter value");

        float dvTotal = 0;
        int updateCostFrom = -1;

        for (int i = 0; i < msequence.size(); ++i) {
            ImGui::PushID(i + 1);
            auto& step = msequence.at(i);
            ImGui::Checkbox("##TEST", &step.active); ImGui::SameLine();
            ImGui::BeginDisabled(!step.active);
            if (step.active) {
                dvTotal += step.dv;
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
                    ImGui::Text("%i. Circularize hyperbolic orbit at %s", i, step.refBody2->name);
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
                case MissionPhaseType::MINING: 
                    ImGui::Text("%i. Mining fuel at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::ORBITAL_REFUELING: 
                    ImGui::Text("%i. Orbital refuelling %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
            }
            if (step.dv > 0) {
                ImGui::Text("Δv: %.0f m/s", step.dv);
            }
            ImGui::EndDisabled();
            ImGui::PopID();
            ImGui::Separator();
            ImGui::Unindent();
        }
        ImGui::TextColored({0.5, 0.5, 1, 1}, "Total Δv: %.2f m/s", dvTotal);
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

    return endWin;
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
                msequence.push_back(MissionPhase { MissionPhaseType::ESCAPE, from, nullptr, startOrbit, destOrbit });
                if (to->orbit.inclination != from->orbit.inclination) {
                    msequence.push_back(MissionPhase { MissionPhaseType::INCLINATION_CORRECTION, from, to, startOrbit });
                }
            }
            if (to == from->orbit.parent) {
                // Return from natural satellite. Need to escape first
                msequence.push_back(MissionPhase { MissionPhaseType::ESCAPE, from, nullptr, startOrbit, reentryPE });
            }
            if (to->orbit.parent == from) {
                // Parent to moon — inclination correction w.r.t. parent's equator
                msequence.push_back(MissionPhase { MissionPhaseType::INCLINATION_CORRECTION, from, to, startOrbit });
            }
            msequence.push_back(MissionPhase::hohmann(from, to, startOrbit, destOrbit));
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
        case MissionPhaseType::LANDING: dv = naiveTakeoffLandingCost(refBody, alt1); break;
        case MissionPhaseType::TAKEOFF: dv = naiveTakeoffLandingCost(refBody, alt1); break;
        case MissionPhaseType::ESCAPE:
            dv = circularizeHyperbolicCost(refBody->orbit.parent, refBody, alt2, alt1, true);
            break;
        case MissionPhaseType::CIRCULARIZE_HYPERBOLIC:
            if (refBody2 && refBody2 == refBody->orbit.parent) {
                dv = circularizeHyperbolicCost(refBody2, refBody, alt2, alt1, true);
            } else if (refBody2 && refBody2->orbit.parent == refBody->orbit.parent) {
                // Planet→planet
                dv = 0;
            } else {
                dv = circularizeHyperbolicCost(refBody, refBody2, alt1, alt2, true);
            }
            break;
        case MissionPhaseType::HOHMANN_TRANSFER:
            if (refBody2 && refBody2 == refBody->orbit.parent) {
                dv = 0;  // ESCAPE already set up the transfer
            } else {
                dv = hohmannTransferCost(refBody, refBody2, alt1);
            }
            break;
        case MissionPhaseType::INCLINATION_CORRECTION:
            if (refBody2 && refBody2->orbit.parent == refBody) {
                // Planet → moon: ship's parking orbit vs moon's orbit around planet
                Orbit shipOrbit = Orbit::circular(refBody, alt1 + refBody->radius_km);
                dv = inclinationCorrectionCost(shipOrbit, refBody2->orbit);
            } else {
                // Planet → planet: orbits around shared parent
                dv = inclinationCorrectionCost(refBody->orbit, refBody2->orbit);
            }
            break;
        default: dv = 0;
    }
}
