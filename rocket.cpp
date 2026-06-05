#include "rocket.h"

#include "LoadKspParts.h"

#include <iostream>

void Rocket::setRootPart(std::string_view partName) {
    for (auto& part : partCatalogue) {
        if (part.name == partName) {
            root = &part;
            return;
        }
    }
    throw std::runtime_error("Part not found in catalogue: " + std::string(partName));
}

Rocket::Rocket(const std::string& rocket_name) : name(rocket_name) { }

void Rocket::loadPartCatalogue(const std::filesystem::path& path) {
    loadPartCatalogueFromKSP(path, partCatalogue);

    for (const auto& part : partCatalogue) {
        part.print();
    }
}
