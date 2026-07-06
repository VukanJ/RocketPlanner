#ifndef GUI_H
#define GUI_H


#include "WindowSimulator.h"
#include "WindowMission.h"
#include <memory>

class GUI {
public:
    GUI(const PartInfoList& engines);
    std::unique_ptr<WindowMission> winM;
    std::unique_ptr<WindowSimulator> winSim;

    void onWindowResized(int width, int height);
    void drawMenuBar();

    void render();

    enum class Phase {MISSION_SELECT, ROCKET} phase = Phase::MISSION_SELECT;
    bool showDemo = false;
};


#endif // GUI_H
