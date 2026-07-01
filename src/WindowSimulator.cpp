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

static const WindowSimulator::PlotDesc s_plots[] = {
    {"Altitude",          "t [s]",     "Altitude (km)",   &FlightData<float>::t,           &FlightData<float>::altitude_km},
    {"Velocity",          "t [s]",     "Velocity (m/s)",  &FlightData<float>::t,           &FlightData<float>::velocity_ms},
    {"Apoapsis",          "t [s]",     "Apoapsis (km)",   &FlightData<float>::t,           &FlightData<float>::apoapsis_km,   nullptr, false, true},
    {"Thrust",            "t [s]",     "Thrust (kN)",     &FlightData<float>::t,           &FlightData<float>::thrust_kN,     nullptr, false, true},
    {"Mass",              "t [s]",     "Mass (t)",        &FlightData<float>::t,           &FlightData<float>::mass_t},
    {"Drag",              "t [s]",     "Drag (N)",        &FlightData<float>::t,           &FlightData<float>::drag_N},
    {"Pressure",          "t [s]",     "Pressure (atm)",  &FlightData<float>::t,           &FlightData<float>::pressure_atm},
    {"Pitch (90=Horiz)",  "t [s]",     "Angle (deg)",     &FlightData<float>::t,           &FlightData<float>::dir_angle_deg},
    {"Area",              "t [s]",     "Area (m\u00b2)",  &FlightData<float>::t,           &FlightData<float>::area_m2},
    {"Trajectory",        "X (km)",    "Y (km)",          &FlightData<float>::posx_km,     &FlightData<float>::posy_km},
    {"Drag/Thrust ratio", "t [s]",     "Drag / Thrust",   &FlightData<float>::t,           nullptr,
        [](const FlightData<float>& d, std::vector<float>& out) {
            out.resize(d.t.size());
            for (size_t i = 0; i < d.t.size(); ++i)
                out[i] = d.thrust_kN[i] > 0 ? d.drag_N[i] / (d.thrust_kN[i] * 1000) : 0;
        }, false, true},
    {"Pitch vs Altitude", "Altitude (km)", "Angle (deg)", &FlightData<float>::altitude_km, &FlightData<float>::dir_angle_deg}
};
static constexpr int nPlots = sizeof(s_plots) / sizeof(s_plots[0]);

WindowSimulator::WindowSimulator(const PartInfoList& engines)
    : allEngines(engines), selectedBody(&KspSystem::Kerbin), showPlot(nPlots, false)
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

    showPlot[2] = true;  // Apoapsis on by default
    showPlot[10] = true; // Drag/Thrust ratio on by default

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

    ImGui::Begin("Configure Rocket", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
    if (ImGui::BeginTable("MainSplit", 2, ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 300.0f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2);

        float cellH = ImGui::GetContentRegionAvail().y;
        float infoMin = 40.0f;
        float splitterH = 4.0f;

        if (rocketConfigHeight_ < 0.0f)
            rocketConfigHeight_ = std::clamp(cellH * 0.75f, 100.0f, 300.0f);

        float maxConfig = std::max(cellH - infoMin - splitterH, 0.0f);
        float minConfig = std::min(80.0f, maxConfig);
        rocketConfigHeight_ = std::clamp(rocketConfigHeight_, minConfig, maxConfig);

        ImGui::BeginChild("RocketConfig", ImVec2(-1, rocketConfigHeight_),
                          ImGuiChildFlags_Borders);
        ImGui::Text("Mission Configuration");
        ImGui::BeginTabBar("ConfigTabs", ImGuiTabBarFlags_None);
        if (ImGui::BeginTabItem("Rocket")) {
            StagingConfigMenu();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem((std::string("Body (") + selectedBody->name + ')').c_str())) {
            renderBodySelector();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("GravityTurn")) {
            renderGravityTurnConfig();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::EndChild();

        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            float availW = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursor.x, cursor.y + splitterH * 0.5f),
                ImVec2(cursor.x + availW, cursor.y + splitterH * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

            ImGui::InvisibleButton("##splitter", ImVec2(-1, splitterH));
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::IsItemActive()) {
                rocketConfigHeight_ += ImGui::GetIO().MouseDelta.y;
                rocketConfigHeight_ = std::clamp(rocketConfigHeight_, minConfig, maxConfig);
            }
            ImGui::PopStyleVar();
        }

        float infoH = std::max(cellH - rocketConfigHeight_ - splitterH, 0.0f);
        {
            ImGui::BeginChild("OrbitalInfo", ImVec2(-1, infoH),
                              ImGuiChildFlags_Borders);
            renderOrbitalSuccessWindow();
            ImGui::EndChild();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

        ImGui::TableNextColumn();

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

        ImGui::EndTable();
    }
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

    for (int s = 0; s < rocket.stages.size(); ++s) {
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
            for (int i = 0; i < allEngines.size(); ++i) {
                if (allEngines[i] == stage.engine) {
                    currentEngineIdx = i;
                    break;
                }
            }

            if (ImGui::BeginCombo("##Engine",
                    stage.engine ? stage.engine->title.c_str() : "None")) {
                for (int i = 0; i < allEngines.size(); ++i) {
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

            bool circ_stage = false;
            if (ImGui::Checkbox("Vacuum stage", &circ_stage)) {
                std::println("WIP");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Whether this stage fires for orbit circularization after reaching vacuum.");
            }

            if (ImGui::SliderInt("##mec", &stage.engineMultiplicity, 1, 4, "Main Engine Count: %i")) {
                configDirty = true;
            }
            if (stage.engineMultiplicity < 0) {
                stage.engineMultiplicity = 0;
            }

            if (ImGui::SliderFloat("##payload", &stage.payloadMass, 0, 20.0, "Freight = %.3f t")) {
                if (stage.payloadMass < 0.0) {
                    stage.payloadMass = 0.0;
                }
                configDirty = true;
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
                int nFrac = asp.fuelFractions.size();
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
            if (s < rocket.stages.size() - 1) {
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

void WindowSimulator::renderGravityTurnConfig() {
    ImGui::Text("Gravity Turn Parameters");

    bool changed = false;
    changed |= ImGui::SliderFloat("Vertical climb (km)", &gtClimbAlt, 0.0f, 70.0f, "%.1f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Altitude to fly straight up before starting the turn.\nEve: 10-20 km, Kerbin: 0-5 km, Duna: 0 km.");
    }

    changed |= ImGui::SliderFloat("Turn aggressiveness", &gtTurnSpread, 0.3f, 3.0f, "%.1f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("How quickly the turn progresses.\n0.3 = aggressive pitch-over, 1.0 = linear, 3.0 = gradual hold-then-snap.\nHigh TWR rockets can handle more aggressive turns.");
    }

    changed |= ImGui::SliderFloat("Final pitch", &gtFinalPitch, 10.0f, 89.0f, "%.0f deg");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Target pitch angle from vertical in vacuum.\n85\u00b0 = 5\u00b0 from horizontal.");
    }

    if (ImGui::Button("Reset to defaults")) {
        gtClimbAlt = 0.0f;
        gtTurnSpread = 1.0f;
        gtFinalPitch = 85.0f;
        changed = true;
    }

    if (changed) { configDirty = true; }
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

    for (int i = 0; i < rocket.stages.size(); ++i) {
        const auto& st = rocket.stages[i];
        const auto& frac = st.asparagus_config.fuelFractions;
        float totalFuel = (float)stageFuelMass[i];
        int sym = st.asparagus_config.baseSymmetry;
        int rings = st.asparagus_config.numAsparagusStages;

        float coreFuel = totalFuel;
        if (rings > 0 && frac.size() == rings + 1) {
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
            float boosterFuel = frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
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

                float boosterFuel = frac.size() > r + 1 ? (float)frac[r + 1] * totalFuel / sym : 0.0f;
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
            else if (i == rocket.stages.size() - 1) { orderLabel = "LAST"; orderColor = IM_COL32(255,160,80,220); }
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
        ImGui::BulletText("Surface g: %.2f m/s²", selectedBody->surfaceGravity);
        ImGui::BulletText("Radius: %.0f km", selectedBody->radius_km);
        if (selectedBody->seaLevel_atm > 0.0f) {
            ImGui::BulletText("Atmosphere: %.4f atm", selectedBody->seaLevel_atm);
            ImGui::BulletText("Atm height: %.0f km", selectedBody->atmHeight_km);
            ImGui::BulletText("Rot. period: %.0f s", selectedBody->rotPeriod_s);
        }
        else {
            ImGui::BulletText("No Atmosphere");
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
        for (int i = 0; i < kinematics.size(); ++i) {
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
        return;
    }
    for (const auto& stage : rocket.stages) {
        if (!stage.engine) {
            return;
        }
    }
    flight_sim.simulate_launch<FlightSimulator::SimOpt::RECORD>(*selectedBody, rocket, gtClimbAlt, gtTurnSpread, gtFinalPitch);
}

void WindowSimulator::renderFlight() {
    if (flight_sim.flight_data.t.empty()) {
        ImGui::TextWrapped("Adjust the rocket configuration to see flight simulation results.");
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.5, 0, 1.0));
    if (ImGui::Button("Save CSV")) {
        std::ofstream csv("flight_data.csv");
        csv << "t,alt_km,vel_m_s,mass_t,thrust_kN,drag_N,apo_km,pressure_atm,dir_deg,area_m2,stage\n";
        for (size_t i = 0; i < flight_sim.flight_data.t.size(); ++i) {
            csv << flight_sim.flight_data.t[i] << ','
                << flight_sim.flight_data.altitude_km[i] << ','
                << flight_sim.flight_data.velocity_ms[i] << ','
                << flight_sim.flight_data.mass_t[i] << ','
                << flight_sim.flight_data.thrust_kN[i] << ','
                << flight_sim.flight_data.drag_N[i] << ','
                << flight_sim.flight_data.apoapsis_km[i] << ','
                << flight_sim.flight_data.pressure_atm[i] << ','
                << flight_sim.flight_data.dir_angle_deg[i] << ','
                << flight_sim.flight_data.area_m2[i] << ','
                << flight_sim.flight_data.stage[i] << '\n';
        }
        std::clog << "CSV saved\n";
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Saves flight_data.csv to working directory");
    }

    auto& data = flight_sim.flight_data;

    for (int i = 0; i < data.t.size(); ++i) {
        if (std::isinf(data.apoapsis_km[i])) {
            data.apoapsis_km[i] = 1e6f;
        }
    }

    std::vector<float> stageTimes;
    for (int i = 1; i < data.stage.size(); ++i) {
        if (data.stage[i] != data.stage[i - 1]) {
            data.t.push_back(data.t[i - 1]);
        }
    }

    float maxThrust = 0;
    for (auto v : data.thrust_kN) {
        if (v > maxThrust) { maxThrust = v; }
    }

    if (ImGui::BeginTable("##plotToggles", 5)) {
        for (int i = 0; i < nPlots; ++i) {
            if (i % 5 == 0) ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool v = showPlot[i];
            ImGui::Checkbox(s_plots[i].label, &v);
            showPlot[i] = v;
        }
        ImGui::EndTable();
    }

    ImGui::Separator();

    ImGui::BeginChild("##plots", ImVec2(-1, -1), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (int p = 0; p < nPlots; ++p) {
        if (!showPlot[p]) { continue; }

        const auto& desc = s_plots[p];

        if (desc.auto_fit_x && desc.auto_fit_y) {
            ImPlot::SetNextAxesToFit();
        } else if (desc.auto_fit_x) {
            ImPlot::SetNextAxisToFit(ImAxis_X1);
        }

        if (ImPlot::BeginPlot(desc.label, ImVec2(-1, 250))) {
            ImPlot::SetupAxes(desc.x_label, desc.y_label);

            if (p == 2 && selectedBody && selectedBody->seaLevel_atm > 0) {
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, selectedBody->atmHeight_km + 20.0f);
            }
            if (p == 3) {
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, maxThrust * 1.1f);
            }

            const float* x_data = (data.*(desc.x_field)).data();
            const float* y_data = nullptr;
            std::vector<float> scratch;

            if (desc.y_field) {
                y_data = (data.*(desc.y_field)).data();
            } else if (desc.compute_y) {
                desc.compute_y(data, scratch);
                y_data = scratch.data();
            }

            ImPlot::PlotLine(desc.label, x_data, y_data, data.t.size());

            if (!stageTimes.empty()) { ImPlot::PlotInfLines("Stage", stageTimes.data(), (int)stageTimes.size()); }
            ImPlot::EndPlot();
        }
    }

    ImGui::EndChild();

}

void WindowSimulator::renderOrbitalSuccessWindow() {
    auto colorText = [](bool cond){
        if (cond) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        }
    };

    ImGui::Text("Launch Information");
    auto& lsuccess = flight_sim.launchSuccess;
    colorText(lsuccess.apoapsis_safe_height);
    ImGui::BulletText("Vacuum Apoapsis? %s", lsuccess.apoapsis_safe_height ? "Yes" : "No");
    ImGui::PopStyleColor();

    bool canCircularize = lsuccess.circularization_dv <= lsuccess.availableDeltaV;
    colorText(canCircularize);
    ImGui::BulletText("Can circularize? %s", canCircularize ? "Yes" : "No");
    ImGui::Indent();
    ImGui::BulletText("Δv available %f m/s", lsuccess.availableDeltaV);

    ImGui::BulletText("Δv needed %f m/s", lsuccess.circularization_dv);
    ImGui::BulletText("Δv left %f m/s", lsuccess.availableDeltaV - lsuccess.circularization_dv);
    ImGui::PopStyleColor();

    ImGui::Unindent();
}

void WindowSimulator::renderRawData() {
    const std::size_t size = flight_sim.flight_data.t.size();
    if (size == 0) return;

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

        int step = std::max(1, static_cast<int>(size) / 100);
        for (int i = 0; i < size; i += step) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f", flight_sim.flight_data.t[i]);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", flight_sim.flight_data.altitude_km[i]);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", flight_sim.flight_data.velocity_ms[i]);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", flight_sim.flight_data.mass_t[i]);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f", flight_sim.flight_data.thrust_kN[i]);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.1f", flight_sim.flight_data.drag_N[i]);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%.1f", flight_sim.flight_data.apoapsis_km[i]);
            ImGui::TableSetColumnIndex(7); ImGui::Text("%.4f", flight_sim.flight_data.pressure_atm[i]);
            ImGui::TableSetColumnIndex(8); ImGui::Text("%.1f", flight_sim.flight_data.dir_angle_deg[i]);
            ImGui::TableSetColumnIndex(9); ImGui::Text("%d",   flight_sim.flight_data.stage[i]);
        }
        ImGui::EndTable();
    }
}
