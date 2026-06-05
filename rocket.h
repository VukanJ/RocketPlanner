#ifndef ROCKET_H
#define ROCKET_H

#include <string_view>
#include <vector>
#include <string>
#include <filesystem>

#include "parts.h"

class Rocket {
public:
    Rocket(const std::string& rocket_name);

    void loadPartCatalogue(const std::filesystem::path& path);

    std::vector<Part> partCatalogue;

    std::vector<const Part*> tanks_LOX;
    std::vector<const Part*> tanks_LF;
    std::vector<const Part*> engines_LF;
    std::vector<const Part*> engines_LOX;
    std::vector<const Part*> engines_Booster;
    std::vector<const Part*> command_modules;

    const std::string name;

    double deltaV() const;
    double mass() const;

    void setRootPart(std::string_view partName);

private:
    Part* root = nullptr;
};

#endif // ROCKET_H
