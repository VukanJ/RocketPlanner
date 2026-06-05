#include "rocket.h"

#include "LoadKspParts.h"

#include <iostream>

void Rocket::setRootPart(std::string_view partName) {
    for (auto& part : partCatalogue) {
        if (part.title == partName) {
            root = &part;
            return;
        }
    }
    throw std::runtime_error("Part not found in catalogue: " + std::string(partName));
}

Rocket::Rocket(const std::string& rocket_name) : name(rocket_name) { }

void Rocket::loadPartCatalogue(const std::filesystem::path& path) {
    loadPartCatalogueFromKSP(path, partCatalogue);
    // Catalogue is constant from here on

    for (const auto& part : partCatalogue) {
        part.print();
        switch (part.type) {
            case PartType::MPTank:           tanks_MP.push_back(&part); break;
            case PartType::LFTank:           tanks_LF.push_back(&part); break;
            case PartType::LFOXTank:         tanks_LOX.push_back(&part); break;
            case PartType::LFEngine:         engines_LF.push_back(&part); break;
            case PartType::LOXEngine:        engines_LOX.push_back(&part); break;
            case PartType::SolidBooster:     engines_Booster.push_back(&part); break;
            case PartType::CrewedCommandPod: command_modules.push_back(&part); break;
            case PartType::DronePod:         command_modules.push_back(&part); break;
            default: break;
        }
    }
}
