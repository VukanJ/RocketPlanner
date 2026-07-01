#ifndef WINDOW_SIMULATOR
#define WINDOW_SIMULATOR

#include "RocketSolver.h"
#include "RocketConfig.h"
#include "FlightSimulator.h"
#include "kspConstants.h"

#include <vector>
#include <string>

class WindowSimulator {
public:
    using FlightDataMember = std::vector<float> FlightData<float>::*;

    struct PlotDesc {
        const char* label;
        const char* x_label;
        const char* y_label;
        FlightDataMember x_field = nullptr;
        FlightDataMember y_field = nullptr;
        void (*compute_y)(const FlightData<float>&, std::vector<float>&) = nullptr;
        bool auto_fit_x = true;
        bool auto_fit_y = true;
    };

    WindowSimulator(const PartInfoList& engines);

    void onWindowResized(int width, int height);

    void render();
    void renderKinematics();
    void renderPictogram();
    void renderBodySelector();
    void renderFlight();
    void renderRawData();
    void renderGravityTurnConfig();
    void renderOrbitalSuccessWindow();

    void StagingConfigMenu();

    int windowWidth = 1280;
    int windowHeight = 720;

    RocketConfig rocket;
    PartInfoList allEngines;
    std::vector<std::string> engineNames;
    std::vector<StageKinematics> kinematics;
    std::vector<double> stageFuelMass;
    const Body* selectedBody;

private:
    bool configDirty = false;
    bool showDemo = false;
    std::vector<bool> showPlot;

    void drawMenuBar();
    void recomputeMasses();
    void updateKinematics();
    void simulateCurrentFlight();

    void insertDefaultStage();

    const PartProperty* defaultEngine;
    FlightSimulator flight_sim;

    float rocketConfigHeight_ = -1.0f;

    float gtClimbAlt = 0.0f;      // km — go straight up this high before turning
    float gtTurnSpread = 1.0f;    // exponent — 0.3=aggressive, 1=linear, 3=gradual
    float gtFinalPitch = 85.0f;   // deg — target pitch from vertical
};

#endif // WINDOW_SIMULATOR
