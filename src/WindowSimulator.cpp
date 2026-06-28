#include "WindowSimulator.h"
#include "imgui.h"
#include "implot.h"
#include <print>
#include <stdexcept>
#include <iostream>
#include <fstream>

static const struct { const char* name; const Body* body; } bodyTable[] = {
    { "Kerbin", &KspSystem::Kerbin },
    { "Eve",    &KspSystem::Eve },
    { "Duna",   &KspSystem::Duna },
    { "Laythe", &KspSystem::Laythe },
    { "Mun",    &KspSystem::Mun },
    { "Minmus", &KspSystem::Minmus },
    { "Moho",   &KspSystem::Moho },
    { "Gilly",  &KspSystem::Gilly },
    { "Dres",   &KspSystem::Dres },
    { "Ike",    &KspSystem::Ike },
    { "Vall",   &KspSystem::Vall },
    { "Tylo",   &KspSystem::Tylo },
    { "Bop",    &KspSystem::Bop },
    { "Pol",    &KspSystem::Pol },
    { "Eeloo",  &KspSystem::Eeloo },
};

WindowSimulator::WindowSimulator(const PartInfoList& engines)
    : allEngines(engines), selectedBody(&KspSystem::Kerbin)
{
    engineNames.reserve(allEngines.size());
    for (const auto* e : allEngines) {
        engineNames.push_back(e->title);
        if (e->title.contains("Vector")) {
            defaultEngine = e;
        }
    }

    if (defaultEngine == nullptr) {
        throw std::runtime_error("Faulty engine list");
    }

    selectedBody = &KspSystem::Kerbin;
    insertDefaultStage();
}

void WindowSimulator::insertDefaultStage() {
    rocket.stages.emplace_back();
    stageFuelMass.push_back(30.0);
    rocket.stages.back().engine = defaultEngine;
    rocket.stages.back().engineMultiplicity = 1;
    configDirty = true;
}

void WindowSimulator::onWindowResized(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void WindowSimulator::drawMenuBar() {
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

void WindowSimulator::render() {
    drawMenuBar();

    float mbh = ImGui::GetFrameHeight();
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight - mbh));
    ImGui::SetNextWindowPos(ImVec2(0, mbh));

    ImGui::Begin("Configure Rocket", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2);

    ImGui::BeginChild("RocketConfig", ImVec2(-1, -1), 
                      ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    ImGui::BeginTabBar("ConfigTabs", ImGuiTabBarFlags_None);
    if (ImGui::BeginTabItem("Rocket")) {
        StagingConfigMenu();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Body")) {
        renderBodySelector();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2);
    
    ImGui::BeginChild("Flight Simulation", ImVec2(-1, -1), 
                      ImGuiChildFlags_Borders);
    ImGui::BeginTabBar("SimTabs", ImGuiTabBarFlags_None);

    if (ImGui::BeginTabItem("Stage Kinematics")) {
        renderKinematics();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Plots")) {
        renderFlight();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Raw data")) {
        renderRawData();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (configDirty) {
        recomputeMasses();
        updateKinematics();
        simulateCurrentFlight();
        configDirty = false;
    }

    ImGui::End();

    if (showDemo) {
        ImGui::ShowDemoWindow(&showDemo);
    }
}

void WindowSimulator::StagingConfigMenu() {
    if (ImGui::Button("+ Add Stage")) {
        insertDefaultStage();
    }

    for (int s = 0; s < (int)rocket.stages.size(); ++s) {
        auto& stage = rocket.stages[s];
        ImGui::PushID(s);
        ImGui::Separator();

        std::string headerLabel = "Stage " + std::to_string(s + 1)
            + " Mass= " + std::to_string(stage.fullMass) + " t";
        if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
            {
                ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                ImVec4 lighter = ImVec4(
                    bg.x + (1.0f - bg.x) * 0.1f,
                    bg.y + (1.0f - bg.y) * 0.1f,
                    bg.z + (1.0f - bg.z) * 0.1f,
                    bg.w);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, lighter);
            }
            ImGui::BeginChild("##Stage", ImVec2(-1, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
            int currentEngineIdx = 0;
            for (int i = 0; i < (int)allEngines.size(); ++i) {
                if (allEngines[i] == stage.engine) {
                    currentEngineIdx = i;
                    break;
                }
            }

            if (ImGui::BeginCombo("##Engine",
                    stage.engine ? stage.engine->title.c_str() : "None")) {
                for (int i = 0; i < (int)allEngines.size(); ++i) {
                    bool selected = (currentEngineIdx == i);
                    if (ImGui::Selectable(allEngines[i]->title.c_str(), selected)) {
                        stage.engine = allEngines[i];
                        configDirty = true;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::SliderInt("##mec", &stage.engineMultiplicity, 1, 4, "Main Engine Count: %i")) {
                configDirty = true;
            }
            if (stage.engineMultiplicity < 0) {
                stage.engineMultiplicity = 0;
            }

            if (ImGui::InputDouble("##fuel", &stageFuelMass[s], 0.1, 1.0, "Fuel = %.3f t")) {
                if (stageFuelMass[s] < 0.0) {
                    stageFuelMass[s] = 0.0;
                }
                configDirty = true;
            }

            auto& asp = stage.asparagus_config;
            if (ImGui::SliderInt("##radsymm", &asp.baseSymmetry, 1, MAX_ASPARAGUS_SYMMETRY, "Radial Symmetry: %i")) {
                if (asp.baseSymmetry == 1) {
                    asp.numAsparagusStages = 0;
                    asp.fuelFractions = {1.0};
                }
                configDirty = true;
            }

            int maxStages = asp.baseSymmetry > 0 ? MAX_ASPARAGUS_SUBSTAGES : 0;
            if (asp.baseSymmetry > 1) {
                if (ImGui::SliderInt("##substages", &asp.numAsparagusStages, 0, maxStages, "Radial stages: %i")) {
                    if (asp.baseSymmetry == 0) {
                        asp.numAsparagusStages = 0;
                    }
                    asp.fuelFractions.assign(asp.numAsparagusStages + 1, 1.0 / (asp.numAsparagusStages + 1));
                    configDirty = true;
                }
            }

            std::uint16_t mask = (1 << asp.numAsparagusStages) - 1;
            asp.hasEngine &= mask;
            for (int a = 0; a < asp.numAsparagusStages; ++a) {
                bool has = (asp.hasEngine >> a) & 1;
                if (ImGui::Checkbox(("Stage " + std::to_string(a + 1) + " has engine").c_str(), &has)) {
                    if (has) {
                        asp.hasEngine |= (1 << a);
                    } else {
                        asp.hasEngine &= ~(1 << a);
                    }
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
                for (int j = 0; j < nFrac; ++j) {
                    sum += asp.fuelFractions[j];
                }

                if (ImGui::Button("Equalize Fuel Fractions")) {
                    for (int j = 0; j < nFrac; ++j) {
                        asp.fuelFractions[j] = 1.0 / nFrac;
                    }
                    configDirty = true;
                }

                for (int f = 0; f < nFrac; ++f) {
                    float val = (float)asp.fuelFractions[f];
                    if (ImGui::SliderFloat(("##frac " + std::to_string(f)).c_str(), &val, 0.0f, 1.0f, "frac = %.3f")) {
                        double oldVal = asp.fuelFractions[f];
                        asp.fuelFractions[f] = val;
                        if (asp.fuelFractions[f] < 0.0) {
                            asp.fuelFractions[f] = 0.0;
                        }
                        double diff = asp.fuelFractions[f] - oldVal;
                        double others = sum - oldVal;
                        if (others > 0.0) {
                            for (int j = 0; j < nFrac; ++j) {
                                if (j == f) {
                                    continue;
                                }
                                double frac = asp.fuelFractions[j] / others;
                                asp.fuelFractions[j] -= diff * frac;
                                if (asp.fuelFractions[j] < 0.0) {
                                    asp.fuelFractions[j] = 0.0;
                                }
                            }
                        }
                        sum = 0.0;
                        for (int j = 0; j < nFrac; ++j) {
                            sum += asp.fuelFractions[j];
                        }
                        if (sum > 0.0) {
                            for (int j = 0; j < nFrac; ++j) {
                                asp.fuelFractions[j] /= sum;
                            }
                            sum = 1.0;
                        }
                        configDirty = true;
                    }
                }

                ImGui::Text("Sum: %.3f", sum);
            }

            ImGui::Spacing();
            if (ImGui::Button("Remove stage")) {
                rocket.stages.erase(rocket.stages.begin() + s);
                stageFuelMass.erase(stageFuelMass.begin() + s);
                configDirty = true;
                ImGui::PopStyleVar();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::EndChild();
                ImGui::Unindent();
                ImGui::PopID();
                --s;
                continue;
            }
            if (s > 0) {
                ImGui::SameLine();
                if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                    std::swap(rocket.stages[s], rocket.stages[s - 1]);
                    std::swap(stageFuelMass[s], stageFuelMass[s - 1]);
                    configDirty = true;
                }
            }
            if (s < (int)rocket.stages.size() - 1) {
                ImGui::SameLine();
                if (ImGui::ArrowButton("##dn", ImGuiDir_Down)) {
                    std::swap(rocket.stages[s], rocket.stages[s + 1]);
                    std::swap(stageFuelMass[s], stageFuelMass[s + 1]);
                    configDirty = true;
                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                    ImGui::EndChild();
                    ImGui::Unindent();
                    ImGui::PopID();
                    --s;
                    continue;
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::Unindent();
        }
        ImGui::PopID();
    }
}

void WindowSimulator::renderPictogram() {
    ImGui::Begin("Rocket Pictogram");
    if (rocket.stages.empty()) { ImGui::End(); return; }

    float maxFuel = 0.0f;
    for (auto& f : stageFuelMass) {
        if ((float)f > maxFuel) {
            maxFuel = (float)f;
        }
    }
    if (maxFuel <= 0.0f) {
        maxFuel = 1.0f;
    }

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
        if (rings > 0 && (int)frac.size() == rings + 1) {
            coreFuel = (float)frac[0] * totalFuel;
        }

        float coreR = std::sqrt(coreFuel / 3.14159f) * coreScale;
        if (coreR < 8.0f) {
            coreR = 8.0f;
        }

        float maxExtent = coreR;
        float prevR = coreR;
        for (int r = 0; r < rings; ++r) {
            if (sym < 2) { break; }
            float boosterFuel = (int)frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
            float boosterR = std::sqrt(boosterFuel / 3.14159f) * coreScale;
            if (boosterR < 4.0f) { boosterR = 4.0f; }
            float minRing = prevR + boosterR + 2.0f;
            if (sym > 1) {
                float minByAngle = boosterR / std::sin(3.14159f / sym);
                if (minByAngle > minRing) { minRing = minByAngle; }
            }
            float ringRad = minRing;
            prevR = ringRad + boosterR;
            float extent = ringRad + boosterR;
            if (extent > maxExtent) { maxExtent = extent; }
        }

        float dim = (maxExtent + 10.0f) * 2.0f;
        ImGui::PushID(i);
        ImGui::BeginChild(i + 1, ImVec2(dim + 80.0f, dim));
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
                if (sym < 2) { break; }
                bool hasEngine = (st.asparagus_config.hasEngine >> (rings - 1 - r)) & 1;
                ImU32 bCol = hasEngine ? IM_COL32(200,200,80,255) : IM_COL32(120,120,120,255);

                float boosterFuel = (int)frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
                float boosterR = std::sqrt(boosterFuel / 3.14159f) * coreScale;
                if (boosterR < 4.0f) { boosterR = 4.0f; }

                float minRing = prevR + boosterR + 2.0f;
                if (sym > 1) {
                    float minByAngle = boosterR / std::sin(3.14159f / sym);
                    if (minByAngle > minRing) { minRing = minByAngle; }
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
        ImGui::EndChild();
        ImGui::PopID();
    }

    ImGui::End();
}

void WindowSimulator::renderBodySelector() {
    const char* currentName = "None";
    for (auto& entry : bodyTable) {
        if (selectedBody == entry.body) {
            currentName = entry.name; break;
        }
    }
    if (ImGui::BeginCombo("Body", currentName)) {
        for (auto& entry : bodyTable) {
            bool isSelected = selectedBody == entry.body;
            if (ImGui::Selectable(entry.name, isSelected)) {
                selectedBody = entry.body;
                configDirty = true;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (selectedBody) {
        ImGui::Text("Surface g: %.2f m/s²", selectedBody->surfaceGravity);
        ImGui::Text("Radius: %.0f km", selectedBody->radius_km);
        ImGui::Text("Atmosphere: %.4f atm", selectedBody->seaLevel_atm);
        if (selectedBody->seaLevel_atm > 0.0f) {
            ImGui::Text("Atm height: %.0f km", selectedBody->atmHeight_km);
            ImGui::Text("Rot. period: %.0f s", selectedBody->rotPeriod_s);
        }
    }
}

void WindowSimulator::renderKinematics() {
    if (ImGui::BeginTable("kinematics", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("#Stage");
        ImGui::TableSetupColumn("m0 [t]");
        ImGui::TableSetupColumn("mf [t]");
        ImGui::TableSetupColumn("burn [s]");
        ImGui::TableSetupColumn("dV [m/s]");
        ImGui::TableSetupColumn("TWR");
        ImGui::TableSetupColumn("Airstream Area [m2]");
        ImGui::TableHeadersRow();

        double totalDV = 0;
        double totalBurn = 0;
        for (int i = 0; i < (int)kinematics.size(); ++i) {
            const auto& k = kinematics[i];
            double dV = k.engine ? k.engine->enginePerf.vacuumISP * 9.81f * std::log(k.m0 / k.mf) : 0.0f;
            double g0 = selectedBody ? selectedBody->surfaceGravity : 9.81f;
            double twr = k.engine ? k.engine->MaxThrustkN * k.nEngines / (k.m0 * g0) : 0.0;
            totalDV += dV;
            totalBurn += k.burnTime;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i + 1);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f", k.m0);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", k.mf);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", k.burnTime);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.0f", dV);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.2f", twr);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%.3f", k.area_m2);
        }
        ImGui::EndTable();
        ImGui::Text("Total Δv = %f m/s", totalDV);
        ImGui::Text("Total burn = %f s", totalBurn);
    }
}

void WindowSimulator::recomputeMasses() {
    stageFuelMass.resize(rocket.stages.size(), 0.0);
    rocket.recomputeMasses(stageFuelMass);
}

void WindowSimulator::updateKinematics() {
    kinematics.clear();
    for (const auto& stage : rocket.stages) {
        if (!stage.engine) { return; }
    }
    rocket.calcStageKinematics(kinematics);
}

void WindowSimulator::simulateCurrentFlight() {
    if (!selectedBody || rocket.stages.empty()) {
        flightData = {};
        return;
    }
    for (const auto& stage : rocket.stages) {
        if (!stage.engine) {
            flightData = {};
            return;
        }
    }
    flightData = simulate_flight(*selectedBody, rocket);
}

void WindowSimulator::renderFlight() {
    if (flightData.t.empty()) {
        ImGui::TextWrapped("Adjust the rocket configuration to see flight simulation results.");
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.5, 0, 1.0));
    if (ImGui::Button("Save CSV")) {
        std::ofstream csv("flight_data.csv");
        csv << "t,alt_km,vel_m_s,mass_t,thrust_kN,drag_N,apo_km,pressure_atm,dir_deg,area_m2,stage\n";
        for (size_t i = 0; i < flightData.t.size(); ++i) {
            csv << flightData.t[i] << ','
                << flightData.altitude_km[i] << ','
                << flightData.velocity_ms[i] << ','
                << flightData.mass_t[i] << ','
                << flightData.thrust_kN[i] << ','
                << flightData.drag_N[i] << ','
                << flightData.apoapsis_km[i] << ','
                << flightData.pressure_atm[i] << ','
                << flightData.dir_angle_deg[i] << ','
                << flightData.area_m2[i] << ','
                << flightData.stage[i] << '\n';
        }
        std::clog << "CSV saved\n";
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Saves flight_data.csv to working directory");
    }

    for (int i = 0; i < (int)flightData.t.size(); ++i) {
        if (std::isinf(flightData.apoapsis_km[i])) {
            flightData.apoapsis_km[i] = 1e6f;
        }
    }

    std::vector<float> stageTimes;
    for (int i = 1; i < (int)flightData.stage.size(); ++i) {
        if (flightData.stage[i] != flightData.stage[i - 1]) {
            stageTimes.push_back(flightData.t[i - 1]);
        }
    }

    float maxThrust = 0;
    for (auto v : flightData.thrust_kN) {
        if (v > maxThrust) { maxThrust = v; }
    }

    static const char* plotLabels[] = {
        "Altitude", "Velocity", "Apoapsis", "Thrust", "Mass",
        "Drag", "Pressure", "Direction", "Area", "Trajectory",
        "Drag/Thrust ratio"
    };
    constexpr int nPlots = sizeof(plotLabels) / sizeof(plotLabels[0]);

    if (ImGui::BeginTable("##plotToggles", 5)) {
        for (int i = 0; i < nPlots; ++i) {
            if (i % 5 == 0) ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Checkbox(plotLabels[i], &showPlot[i]);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();

    ImGui::BeginChild("##plots", ImVec2(-1, -1), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (int p = 0; p < nPlots; ++p) {
        if (!showPlot[p]) continue;

        if (p == 2 || p == 3)
            ImPlot::SetNextAxisToFit(ImAxis_X1);
        else
            ImPlot::SetNextAxesToFit();

        if (ImPlot::BeginPlot(plotLabels[p], ImVec2(-1, 250))) {
            switch (p) {
                case 0:
                    ImPlot::SetupAxes("Time (s)", "Altitude (km)");
                    ImPlot::PlotLine("Alt", flightData.t.data(), flightData.altitude_km.data(), (int)flightData.t.size());
                    break;
                case 1:
                    ImPlot::SetupAxes("Time (s)", "Velocity (m/s)");
                    ImPlot::PlotLine("V", flightData.t.data(), flightData.velocity_ms.data(), (int)flightData.t.size());
                    break;
                case 2:
                    ImPlot::SetupAxes("Time (s)", "Apoapsis (km)");
                    if (selectedBody && selectedBody->seaLevel_atm > 0) {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, selectedBody->atmHeight_km + 20.0f);
                    }
                    ImPlot::PlotLine("Apo", flightData.t.data(), flightData.apoapsis_km.data(), (int)flightData.t.size());
                    break;
                case 3:
                    ImPlot::SetupAxes("Time (s)", "Thrust (kN)");
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, maxThrust * 1.1f);
                    ImPlot::PlotLine("T", flightData.t.data(), flightData.thrust_kN.data(), (int)flightData.t.size());
                    break;
                case 4:
                    ImPlot::SetupAxes("Time (s)", "Mass (t)");
                    ImPlot::PlotLine("M", flightData.t.data(), flightData.mass_t.data(), (int)flightData.t.size());
                    break;
                case 5:
                    ImPlot::SetupAxes("Time (s)", "Drag (N)");
                    ImPlot::PlotLine("D", flightData.t.data(), flightData.drag_N.data(), (int)flightData.t.size());
                    break;
                case 6:
                    ImPlot::SetupAxes("Time (s)", "Pressure (atm)");
                    ImPlot::PlotLine("P", flightData.t.data(), flightData.pressure_atm.data(), (int)flightData.t.size());
                    break;
                case 7:
                    ImPlot::SetupAxes("Time (s)", "Angle (deg)");
                    ImPlot::PlotLine("Dir", flightData.t.data(), flightData.dir_angle_deg.data(), (int)flightData.t.size());
                    break;
                case 8:
                    ImPlot::SetupAxes("Time (s)", "Area (m\u00b2)");
                    ImPlot::PlotLine("A", flightData.t.data(), flightData.area_m2.data(), (int)flightData.t.size());
                    break;
                case 9:
                    ImPlot::SetupAxes("X (km)", "Y (km)");
                    ImPlot::PlotLine("Pos", flightData.posx_km.data(), flightData.posy_km.data(), (int)flightData.t.size());
                    break;
                case 10:
                    ImPlot::SetupAxes("Time (s)", "Drag / Thrust");
                    { // thrust_kN in kN, drag_N in N → ratio = thrust_kN * 1000 / drag_N
                        std::vector<float> ratio(flightData.t.size());
                        for (size_t i = 0; i < flightData.t.size(); ++i) {
                            ratio[i] = flightData.thrust_kN[i] > 0 ? flightData.drag_N[i] / (flightData.thrust_kN[i] * 1000.0f) : 0;
                        }
                        ImPlot::PlotLine("D/T", flightData.t.data(), ratio.data(), (int)ratio.size());
                    }
                    break;
            }
            if (!stageTimes.empty()) { ImPlot::PlotInfLines("Stage", stageTimes.data(), (int)stageTimes.size()); }
            ImPlot::EndPlot();
        }
    }

    ImGui::EndChild();

}

void WindowSimulator::renderRawData() {
    if (flightData.t.empty()) return;

    if (ImGui::BeginTable("flight_table", 10, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("t");
        ImGui::TableSetupColumn("alt_km");
        ImGui::TableSetupColumn("vel_m_s");
        ImGui::TableSetupColumn("mass_t");
        ImGui::TableSetupColumn("thrust_kN");
        ImGui::TableSetupColumn("drag_N");
        ImGui::TableSetupColumn("apo_km");
        ImGui::TableSetupColumn("press_atm");
        ImGui::TableSetupColumn("dir_deg");
        ImGui::TableSetupColumn("stage");
        ImGui::TableHeadersRow();

        int step = std::max(1, (int)flightData.t.size() / 100);
        for (int i = 0; i < (int)flightData.t.size(); i += step) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f", flightData.t[i]);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", flightData.altitude_km[i]);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", flightData.velocity_ms[i]);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", flightData.mass_t[i]);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f", flightData.thrust_kN[i]);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.1f", flightData.drag_N[i]);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%.1f", flightData.apoapsis_km[i]);
            ImGui::TableSetColumnIndex(7); ImGui::Text("%.4f", flightData.pressure_atm[i]);
            ImGui::TableSetColumnIndex(8); ImGui::Text("%.1f", flightData.dir_angle_deg[i]);
            ImGui::TableSetColumnIndex(9); ImGui::Text("%d", flightData.stage[i]);
        }
        ImGui::EndTable();
    }
}
