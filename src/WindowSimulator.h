#ifndef WINDOW_SIMULATOR
#define WINDOW_SIMULATOR

#include "RocketSolver.h"
#include "kspConstants.h"

#include <vector>
#include <string>

class WindowSimulator {
public:
    WindowSimulator(const PartInfoList& engines);

    void onWindowResized(int width, int height);

    void render();
    void renderKinematics();
    void renderPictogram();
    void renderBodySelector();
    void renderFlight();
    void renderRawData();

    void StagingConfigMenu();

    int windowWidth = 1280;
    int windowHeight = 720;

    RocketSolver::RocketConfig rocket;
    PartInfoList allEngines;
    std::vector<std::string> engineNames;
    std::vector<StageKinematics> kinematics;
    std::vector<double> stageFuelMass;
    const Body* selectedBody;
    FlightData<float> flightData;

private:
    bool configDirty = false;
    bool showDemo = false;
    bool showPlot[11] = {false, false, true, false, false, false, false, false, false, false, true};

    void drawMenuBar();
    void recomputeMasses();
    void updateKinematics();
    void simulateCurrentFlight();

    void insertDefaultStage();

    const PartProperty* defaultEngine;
};

#endif // WINDOW_SIMULATOR
