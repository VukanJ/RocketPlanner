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
        ImGui::SameLine();
        if (s > 0) {
            if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                std::swap(rocket.stages[s], rocket.stages[s - 1]);
                std::swap(stageFuelMass[s], stageFuelMass[s - 1]);
                configDirty = true;
            }
            ImGui::SameLine();
        }
        if (s < (int)rocket.stages.size() - 1) {
            if (ImGui::ArrowButton("##dn", ImGuiDir_Down)) {
                std::swap(rocket.stages[s], rocket.stages[s + 1]);
                std::swap(stageFuelMass[s], stageFuelMass[s + 1]);
                configDirty = true;
                --s;
                continue;
            }
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

void WindowSimulator::renderPictogram() {
    ImGui::Begin("Rocket Pictogram");
    if (rocket.stages.empty()) { ImGui::End(); return; }

    float maxFuel = 0.0f;
    for (auto& f : stageFuelMass)
        if ((float)f > maxFuel) maxFuel = (float)f;
    if (maxFuel <= 0.0f) maxFuel = 1.0f;

    float coreScale = 50.0f / std::sqrt(maxFuel / 3.14159f);

    ImU32 colors[] = { IM_COL32(200,80,80,255), IM_COL32(80,160,200,255),
                       IM_COL32(160,200,80,255), IM_COL32(200,160,80,255),
                       IM_COL32(160,80,200,255), IM_COL32(80,200,160,255) };

    for (int i = 0; i < (int)rocket.stages.size(); ++i) {
        const auto& st = rocket.stages[i];
        const auto& frac = st.asparagus_config.fuelFractions;
        float totalFuel = (float)stageFuelMass[i];
        int sym = st.asparagus_config.baseSymmetry;
        int rings = st.asparagus_config.numAsparagusStages;

        float coreFuel = totalFuel;
        if (rings > 0 && (int)frac.size() == rings + 1)
            coreFuel = (float)frac[0] * totalFuel;

        float coreR = std::sqrt(coreFuel / 3.14159f) * coreScale;
        if (coreR < 8.0f) coreR = 8.0f;

        float maxExtent = coreR;
        float prevR = coreR;
        for (int r = 0; r < rings; ++r) {
            float boosterFuel = (int)frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
            float boosterR = std::sqrt(boosterFuel / 3.14159f) * coreScale;
            if (boosterR < 4.0f) boosterR = 4.0f;
            float minRing = prevR + boosterR + 2.0f;
            if (sym > 1) {
                float minByAngle = boosterR / std::sin(3.14159f / sym);
                if (minByAngle > minRing) minRing = minByAngle;
            }
            float ringRad = minRing;
            prevR = ringRad + boosterR;
            float extent = ringRad + boosterR;
            if (extent > maxExtent) maxExtent = extent;
        }

        float dim = (maxExtent + 10.0f) * 2.0f;
        ImGui::PushID(i);
        ImGui::BeginChildFrame(i, ImVec2(dim + 80.0f, dim));
        {
            auto* draw = ImGui::GetWindowDrawList();
            ImVec2 origin = ImGui::GetCursorScreenPos();
            float cx = origin.x + maxExtent + 10.0f;
            float cy = origin.y + maxExtent + 10.0f;

            ImU32 col = colors[i % 6];
            draw->AddCircleFilled(ImVec2(cx, cy), coreR, col);
            draw->AddCircle(ImVec2(cx, cy), coreR, IM_COL32(255,255,255,80));

            prevR = coreR;
            for (int r = 0; r < rings; ++r) {
                bool hasEngine = (st.asparagus_config.hasEngine >> r) & 1;
                ImU32 bCol = hasEngine ? IM_COL32(200,200,80,255) : IM_COL32(120,120,120,255);

                float boosterFuel = (int)frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
                float boosterR = std::sqrt(boosterFuel / 3.14159f) * coreScale;
                if (boosterR < 4.0f) boosterR = 4.0f;

                float minRing = prevR + boosterR + 2.0f;
                if (sym > 1) {
                    float minByAngle = boosterR / std::sin(3.14159f / sym);
                    if (minByAngle > minRing) minRing = minByAngle;
                }
                float ringRad = minRing;
                prevR = ringRad + boosterR;
                for (int b = 0; b < sym; ++b) {
                    float angle = (float)b / sym * 2.0f * 3.14159f;
                    float bx = cx + ringRad * std::cos(angle);
                    float by = cy + ringRad * std::sin(angle);
                    draw->AddCircleFilled(ImVec2(bx, by), boosterR, bCol);
                    draw->AddCircle(ImVec2(bx, by), boosterR, IM_COL32(255,255,255,60));
                }
            }

            draw->AddText(ImVec2(cx + coreR + 6, cy - 6), IM_COL32(255,255,255,200),
                ("S" + std::to_string(i + 1)).c_str());

            const char* orderLabel = nullptr;
            ImU32 orderColor;
            if (i == 0) { orderLabel = "FIRST"; orderColor = IM_COL32(80,255,80,220); }
            else if (i == (int)rocket.stages.size() - 1) { orderLabel = "LAST"; orderColor = IM_COL32(255,160,80,220); }
            if (orderLabel) {
                ImVec2 ts = ImGui::CalcTextSize(orderLabel);
                draw->AddText(ImVec2(cx - ts.x * 0.5f, origin.y + 2.0f), orderColor, orderLabel);
            }
        }
        ImGui::EndChildFrame();
        ImGui::PopID();
    }

    ImGui::End();
}

void WindowSimulator::renderKinematics() {
    ImGui::Begin("Stage Kinematics");
    if (ImGui::BeginTable("kinematics", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("m0 [t]");
        ImGui::TableSetupColumn("mf [t]");
        ImGui::TableSetupColumn("burn [s]");
        ImGui::TableSetupColumn("dV [m/s]");
        ImGui::TableSetupColumn("area [m2]");
        ImGui::TableSetupColumn("engine");
        ImGui::TableSetupColumn("nEng");
        ImGui::TableHeadersRow();

        double totalDV = 0;
        for (int i = 0; i < (int)kinematics.size(); ++i) {
            const auto& k = kinematics[i];
            double dV = k.engine ? k.engine->enginePerf.vacuumISP * 9.81f * std::log(k.m0 / k.mf) : 0.0f;
            totalDV += dV;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i + 1);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f", k.m0);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", k.mf);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", k.burnTime);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.0f", dV);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.3f", k.area_m2);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%s", k.engine ? k.engine->title.c_str() : "none");
            ImGui::TableSetColumnIndex(7); ImGui::Text("%d", k.nEngines);
        }
        ImGui::EndTable();
        ImGui::Text("Total Δv = %f m/s", totalDV);
    }
    ImGui::End();
}

void WindowSimulator::recomputeMasses() {
    stageFuelMass.resize(rocket.stages.size(), 0.0);
    rocket.recomputeMasses(stageFuelMass);
}

void WindowSimulator::updateKinematics() {
    kinematics.clear();
    for (const auto& stage : rocket.stages)
        if (!stage.engine) return;
    rocket.calcStageKinematics(kinematics);
}
