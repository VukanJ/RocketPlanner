#ifndef ROCKET_H
#define ROCKET_H

#include <string_view>
#include <vector>
#include <string>
#include <filesystem>

#include "parts.h"

class Rocket {
public:
    Rocket(const std::string& name);

    void loadPartCatalogue(const std::filesystem::path& path);

    std::vector<Part> partCatalogue;

    const std::string name;

    double deltaV() const;
    double mass() const;

    void setRootPart(std::string_view partName);

private:
    Part* root = nullptr;
};

#endif // ROCKET_H
