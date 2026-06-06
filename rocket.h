#ifndef ROCKET_H
#define ROCKET_H

#include <string_view>
#include <vector>
#include <string>
#include <list>
#include <filesystem>

#include "parts.h"
#include "stage.h"

class Rocket {
public:
    Rocket(const std::string& rocket_name);

    void loadPartCatalogue(const std::filesystem::path& path);

    std::vector<PartProperty> partCatalogue;

    std::vector<const PartProperty*> tanks_MP;
    std::vector<const PartProperty*> tanks_LOX;
    std::vector<const PartProperty*> tanks_LF;
    std::vector<const PartProperty*> engines_LF;
    std::vector<const PartProperty*> engines_LOX;
    std::vector<const PartProperty*> engines_Booster;
    std::vector<const PartProperty*> command_modules;

    const std::string name;

    double deltaV() const;
    double mass() const;
    void printConfig() const;

    void setRootPart(std::string_view partName);
    void construct(double targetDeltaV, double payloadMass, double minTWR=0, double g0=Constants::g0_tylo);

private:
    PartProperty* root = nullptr;

    std::vector<Stage> stages;
};

#endif // ROCKET_H
