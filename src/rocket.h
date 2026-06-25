#ifndef ROCKET_H
#define ROCKET_H

#include <string_view>
#include <vector>
#include <string>
#include <filesystem>

#include "parts.h"
#include "stage.h"
#include "RocketSolver.h"

class Rocket {
public:
    Rocket(const std::filesystem::path& kspPath, const std::string& rocket_name);

    const std::string name;

    double deltaV() const;
    double mass() const;
    void printConfig() const;

    void setRootPart(std::string_view partName);
    void construct(double targetDeltaV, double payloadMass, double minTWR=0, double g0=KspSystem::Kerbin.surfaceGravity, double seaLevelAtm=0.0);

    const PartInfoList& allEngines() const { return partCatalogue.all_engines; }

private:
    PartProperty* root = nullptr;
    PartCatalogue partCatalogue;

    std::vector<Stage> stages;
};

#endif // ROCKET_H
