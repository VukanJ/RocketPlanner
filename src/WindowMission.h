#ifndef WINDOW_MISSION
#define WINDOW_MISSION

#include "Calendar.h"
#include "DeltaV.h"
#include "LambertSolver.h"
#include "Mission.h"
#include "SystemMap.h"
#include "ThreadPool.h"
#include "kspConstants.h"

// TODO
// Moon to Foreign Moon
// Planet to Foreign Moon

class WindowMission {
public:
    WindowMission();
    Mission mission;

    bool render();
    void showTransferOrbit(float launchSeconds, float flightSeconds);
    bool renderTimeInput();

    enum class InputPhase { FromTo, Sequence } input_phase = WindowMission::InputPhase::FromTo;

    std::vector<MissionPhase> msequence;
    bool advanced = false;
    bool launchDateOptim = false;
    SystemMap systemMap;

    KerbalDate currentDate { 0 };
    LambertSolver transfer_solver;
    bool update_solver = false;

    static ThreadPool threadPool;
};

#endif // WINDOW_MISSION
