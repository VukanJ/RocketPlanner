#include "WindowMission.h"
#include "gui.h"
#include "utils.h"
#include "imgui.h"
#include <cmath>

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


float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm) {
    float mu = body->GM();
    float R = body->radius_km;
    float Rtarget = body->radius_km + targetAltitudeKm;
    float a = 0.5f * (R + Rtarget);

    float dv = 1000.0f * (std::sqrt(mu * (2.0f / R - 1.0f / a))
                        - std::sqrt(mu * (2.0f / Rtarget - 1.0f / a))
                        + std::sqrt(mu / Rtarget));

    if (body->seaLevel_atm > 0.0f) {
        // Assume rocket gains altitude at terminal velocity while ascending by the atmospheric scaling height.
        // The time spent in the atmosphere can be translated into a gravity loss expression that scales
        // well to all bodies in the KSP system with an atmosphere.
        //
        // v_terminal ∝ sqrt(g / ρ).
        // Calculate time needed to pass one scale height at terminal velocity
        // t_climb = H / v_term
        // Finally, the total accumulated gravity Δv loss during that time is g0 * t_climb
        // This is very crude, but after adding a scale parameter k_atm=50.0f tuned to Kerbins known value of ~3500 m/s 
        // The formula happens to match the published dV maps quite well
        constexpr float k_atm = 50.0f;
        dv += k_atm * std::sqrt(body->surfaceGravity)
                    * body->atm_falloff_km
                    * std::sqrt(body->seaLevel_atm);
    }

    return dv;
}

std::pair<float, float> hohmannTransferCost(const Orbit& A, const Orbit& B) {
    // Calculate dV range for hohmann transfer
    if (A.refBody != B.refBody) {
        // This is not a Hohmann transfer
        return {-1, -1};
    }

    if (A.eccentricity > 1e-2) {
        return {-2, -2}; // Cant handle this yet
    }

    if (A.inclination != B.inclination) {
        return {-3, -3}; // Cant handle this yet
    }

    if ((A.PE > B.AP && A.AP < B.PE) || (B.PE > A.AP && B.AP < A.PE)) {
        // Orbits intersect
        return {-4, -4}; // Cant handle this yet
    }

    // Coplanar orbits, one contains the other. Starting orbit is circular

    // Use vis-viva 
    float speedA = 1000.0 * std::sqrt(A.refBody->GM() / A.AP);
    //float speedB_AP = 1000.0 * std::sqrt(A.refBody->GM() * (2.0 / B.AP - 1.0 / B.a_semi));
    //float speedB_PE = 1000.0 * std::sqrt(A.refBody->GM() * (2.0 / B.PE - 1.0 / B.a_semi));

    // Construct transfer orbits 
    float a_trans_min = 0.5f * (A.AP + B.AP);
    float a_trans_max = 0.5f * (A.AP + B.PE);

    float dv_vmin = 1000.0f * std::sqrt(A.refBody->GM() * (2.0 / A.AP - 1.0 / a_trans_min)) - speedA;
    float dv_vmax = 1000.0f * std::sqrt(A.refBody->GM() * (2.0 / A.AP - 1.0 / a_trans_max)) - speedA;

    return {dv_vmin, dv_vmax};
}

float circularizeHyperbolicCost() {

}

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
                    ImGui::Text("Δv: %.0f", naiveTakeoffLandingCost(step.refBody, step.orbitAltitude));
                    ImGui::Indent();
                    break;
                case MissionPhaseType::CIRCULARIZE_HYPERBOLIC:
                    ImGui::BulletText("%i. Circularize hyperbolic orbit at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::HOHMANN_TRANSFER: 
                    {
                        ImGui::BulletText("%i Transfer %s --> %s", i, step.refBody->name, step.refBody2->name);
                        auto dv_hoh = hohmannTransferCost(Orbit::circular(step.refBody, step.orbitAltitude + step.refBody->radius_km), step.refBody2->orbit);
                        if (std::abs(dv_hoh.second - dv_hoh.first) > 10) {
                            ImGui::Text("Δv: %.0f - %.0f", dv_hoh.first, dv_hoh.second);
                        }
                        else {
                            ImGui::Text("Δv: %.0f", dv_hoh.first);
                        }
                        ImGui::Indent();
                    }
                    break;
                case MissionPhaseType::ATMOSPHERIC_BREAKING: 
                    ImGui::BulletText("%i. Atmospheric breaking at %s", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::LANDING_PARACHUTES: 
                    ImGui::BulletText("%i. Landing on %s with parachutes", i, step.refBody->name);
                    ImGui::Indent();
                    break;
                case MissionPhaseType::LANDING: 
                    ImGui::BulletText("%i. Landing on %s", i, step.refBody->name);
                    ImGui::Text("Δv: %.0f", naiveTakeoffLandingCost(step.refBody, step.orbitAltitude));
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

        msequence.push_back(MissionPhase { MissionPhaseType::TAKEOFF, from, nullptr, startOrbit });
        if (from == to) {
            if (to->atmHeight_km > 0) {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, from, nullptr, startOrbit });
            }
            else {
                msequence.push_back(MissionPhase { MissionPhaseType::LANDING, from, nullptr, destOrbit });
            }
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::HOHMANN_TRANSFER, from, to, startOrbit });
        }
        if (to->atmHeight_km > 0) {
            msequence.push_back(MissionPhase { MissionPhaseType::ATMOSPHERIC_BREAKING, to, nullptr, destOrbit });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING_PARACHUTES, to, nullptr, destOrbit });
        }
        else {
            msequence.push_back(MissionPhase { MissionPhaseType::CIRCULARIZE_HYPERBOLIC, to, nullptr, destOrbit });
            msequence.push_back(MissionPhase { MissionPhaseType::LANDING, to, nullptr, destOrbit });
        }
    };
    fromTo(mission.originBody, mission.destinationBody);
    if (!mission.oneWayTrip) {
        fromTo(mission.destinationBody, mission.originBody);
    }
}


