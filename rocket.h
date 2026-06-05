#ifndef ROCKET_H
#define ROCKET_H

#include <vector>
#include <string>
#include <filesystem>

#include "parts.h"

class Rocket {
public:
    Rocket(const std::string& name);

    void loadPartCatalogue(const std::filesystem::path& path);

    std::vector<Part> usedParts;
    std::vector<Part> partCatalogue;

    const std::string name;

    double deltaV() const;
    double mass() const;

    Part* root = nullptr;
};

#endif // ROCKET_H
