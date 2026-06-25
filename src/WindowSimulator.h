#ifndef WINDOW_SIMULATOR
#define WINDOW_SIMULATOR

#include "RocketSolver.h"

#include <vector>
#include <string>

class WindowSimulator {
public:
    WindowSimulator(const PartInfoList& engines);

    void render();
    void renderKinematics();

    RocketSolver::RocketConfig rocket;
    PartInfoList allEngines;
    std::vector<std::string> engineNames;
    std::vector<StageKinematics> kinematics;
    std::vector<double> stageFuelMass;

private:
    bool configDirty = false;
    void recomputeMasses();
    void updateKinematics();

    void insertDefaultStage();

    const PartProperty* defaultEngine;
};

#endif // WINDOW_SIMULATOR
