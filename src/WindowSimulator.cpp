#include "WindowSimulator.h"
#include "imgui.h"

WindowSimulator::WindowSimulator(const PartInfoList& engines)
    : allEngines(engines)
{
    engineNames.reserve(allEngines.size());
    for (const auto* e : allEngines) {
        engineNames.push_back(e->title);
        if (e->title.contains("Vector")) {
            defaultEngine = e;
        }
    }

    insertDefaultStage();
}

void WindowSimulator::insertDefaultStage() {
    rocket.stages.emplace_back();
    stageFuelMass.push_back(3.0);
    rocket.stages.back().engine = defaultEngine;
    rocket.stages.back().engineMultiplicity = 1;
    configDirty = true;
}

void WindowSimulator::render() {
    ImGui::Begin("Configure Rocket");

    if (ImGui::Button("+ Add Stage")) {
        insertDefaultStage();
    }

    for (int s = 0; s < (int)rocket.stages.size(); ++s) {
        auto& stage = rocket.stages[s];
        ImGui::PushID(s);
        ImGui::Separator();
        ImGui::Text("Stage %i. Full %.3f / Empty %.3f t", s + 1, stage.fullMass, stage.emptyMass);

        int current = 0;
        for (int i = 0; i < (int)allEngines.size(); ++i) {
            if (allEngines[i] == stage.engine) {
                current = i;
                break;
            }
        }

        if (ImGui::BeginCombo(("Engine##" + std::to_string(s)).c_str(), 
                stage.engine ? stage.engine->title.c_str() : "None")) {
            for (int i = 0; i < (int)allEngines.size(); ++i) {
                bool selected = (current == i);
                if (ImGui::Selectable(allEngines[i]->title.c_str(), selected)) {
                    stage.engine = allEngines[i];
                    configDirty = true;
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::InputInt(("Count##" + std::to_string(s)).c_str(), &stage.engineMultiplicity, 1, 4))
            configDirty = true;
        if (stage.engineMultiplicity < 0) stage.engineMultiplicity = 0;

        if (ImGui::InputDouble(("Fuel (t)##" + std::to_string(s)).c_str(), &stageFuelMass[s], 0.1, 1.0, "%.3f")) {
            if (stageFuelMass[s] < 0.0) stageFuelMass[s] = 0.0;
            configDirty = true;
        }

        auto& asp = stage.asparagus_config;
        if (ImGui::CollapsingHeader(("Asparagus##" + std::to_string(s)).c_str())) {
            ImGui::Indent();
            if (ImGui::InputInt("Base symmetry", &asp.baseSymmetry, 1, 1)) {
                if (asp.baseSymmetry < 0) asp.baseSymmetry = 0;
                if (asp.baseSymmetry > MAX_ASPARAGUS_SYMMETRY) asp.baseSymmetry = MAX_ASPARAGUS_SYMMETRY;
                if (asp.baseSymmetry == 0) {
                    asp.numAsparagusStages = 0;
                    asp.fuelFractions = {1.0};
                }
                configDirty = true;
            }

            if (ImGui::InputInt("Asparagus stages", &asp.numAsparagusStages, 1, 1)) {
                if (asp.numAsparagusStages < 0) asp.numAsparagusStages = 0;
                if (asp.baseSymmetry == 0) asp.numAsparagusStages = 0;
                if (asp.numAsparagusStages > MAX_ASPARAGUS_SUBSTAGES) asp.numAsparagusStages = MAX_ASPARAGUS_SUBSTAGES;
                asp.fuelFractions.assign(asp.numAsparagusStages + 1, 1.0 / (asp.numAsparagusStages + 1));
                configDirty = true;
            }

            std::uint16_t mask = (1 << asp.numAsparagusStages) - 1;
            asp.hasEngine &= mask;

            for (int a = 0; a < asp.numAsparagusStages; ++a) {
                bool has = (asp.hasEngine >> a) & 1;
                if (ImGui::Checkbox(("Stage " + std::to_string(a + 1) + " has engine").c_str(), &has)) {
                    if (has)
                        asp.hasEngine |= (1 << a);
                    else
                        asp.hasEngine &= ~(1 << a);
                    configDirty = true;
                }
            }

            if (asp.numAsparagusStages > 0) {
                int nFrac = (int)asp.fuelFractions.size();
                if (nFrac != asp.numAsparagusStages + 1) {
                    asp.fuelFractions.assign(asp.numAsparagusStages + 1, 1.0 / (asp.numAsparagusStages + 1));
                    nFrac = asp.numAsparagusStages + 1;
                }

                double sum = 0.0;
                for (int j = 0; j < nFrac; ++j) sum += asp.fuelFractions[j];

                if (ImGui::Button(("Equal##" + std::to_string(s)).c_str())) {
                    for (int j = 0; j < nFrac; ++j) asp.fuelFractions[j] = 1.0 / nFrac;
                    configDirty = true;
                }

                for (int f = 0; f < nFrac; ++f) {
                    float val = (float)asp.fuelFractions[f];
                    if (ImGui::SliderFloat(("Frac " + std::to_string(f + 1)).c_str(),
                            &val, 0.0f, 1.0f, "%.3f")) {
                        double oldVal = asp.fuelFractions[f];
                        asp.fuelFractions[f] = val;
                        if (asp.fuelFractions[f] < 0.0) asp.fuelFractions[f] = 0.0;
                        double diff = asp.fuelFractions[f] - oldVal;
                        double others = sum - oldVal;
                        if (others > 0.0) {
                            for (int j = 0; j < nFrac; ++j) {
                                if (j == f) continue;
                                double frac = asp.fuelFractions[j] / others;
                                asp.fuelFractions[j] -= diff * frac;
                                if (asp.fuelFractions[j] < 0.0) asp.fuelFractions[j] = 0.0;
                            }
                        }
                        configDirty = true;
                        sum = 0.0;
                        for (int j = 0; j < nFrac; ++j) sum += asp.fuelFractions[j];
                    }
                }

                ImGui::Text("Sum: %.3f", sum);
            }
            ImGui::Unindent();
        }

        if (ImGui::Button("Remove stage")) {
            rocket.stages.erase(rocket.stages.begin() + s);
            stageFuelMass.erase(stageFuelMass.begin() + s);
            configDirty = true;
            ImGui::PopID();
            --s;
            continue;
        }
        ImGui::PopID();
    }

    if (configDirty) {
        recomputeMasses();
        updateKinematics();
        configDirty = false;
    }

    ImGui::End();
}

void WindowSimulator::renderKinematics() {
    ImGui::Begin("Stage Kinematics");
    if (ImGui::BeginTable("kinematics", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("m0 [t]");
        ImGui::TableSetupColumn("mf [t]");
        ImGui::TableSetupColumn("burn [s]");
        ImGui::TableSetupColumn("area [m2]");
        ImGui::TableSetupColumn("engine");
        ImGui::TableSetupColumn("nEng");
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)kinematics.size(); ++i) {
            const auto& k = kinematics[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i + 1);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f", k.m0);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", k.mf);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", k.burnTime);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f", k.area_m2);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%s", k.engine ? k.engine->title.c_str() : "none");
            ImGui::TableSetColumnIndex(6); ImGui::Text("%d", k.nEngines);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void WindowSimulator::recomputeMasses() {
    stageFuelMass.resize(rocket.stages.size(), 0.0);

    double belowMass = 0.0;
    for (int i = rocket.stages.size() - 1; i >= 0; --i) {
        auto& stage = rocket.stages[i];
        double enginesMass = 0.0;
        if (stage.engine) {
            enginesMass = stage.engine->getMass() * stage.engineMultiplicity;
            auto& asp = stage.asparagus_config;
            if (asp.baseSymmetry > 0 && asp.numAsparagusStages > 0) {
                int boosters = std::popcount((unsigned int)asp.hasEngine) * asp.baseSymmetry;
                enginesMass += stage.engine->getMass() * boosters;
            }
        }
        stage.emptyMass = enginesMass + belowMass;
        stage.fullMass = stage.emptyMass + stageFuelMass[i];
        if (stage.asparagus_config.baseSymmetry == 0)
            stage.asparagus_config.fuelFractions = {1.0};
        else if ((int)stage.asparagus_config.fuelFractions.size() != stage.asparagus_config.numAsparagusStages + 1)
            stage.asparagus_config.fuelFractions.assign(stage.asparagus_config.numAsparagusStages + 1, 1.0 / (stage.asparagus_config.numAsparagusStages + 1));
        belowMass = stage.fullMass;
    }
}

void WindowSimulator::updateKinematics() {
    kinematics.clear();
    for (const auto& stage : rocket.stages)
        if (!stage.engine) return;
    rocket.calcStageKinematics(kinematics);
}
