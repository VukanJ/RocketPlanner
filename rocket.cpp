#include "rocket.h"

#include "LoadKspParts.h"

std::string partTypeToString(PartType type) {
    switch (type) {
        case PartType::CrewedCommandPod: return "Crewed Command Pod";
        case PartType::DronePod: return "DronePod";
        case PartType::LFTank: return "LF-Tank";
        case PartType::LFOXTank: return "LOX-Tank";
        case PartType::MPTank: return "MonoProp-Tank";
        case PartType::XenonTank: return "Xenon-Tank";
        case PartType::LFEngine: return "LF-Engine";
        case PartType::LOXEngine: return "LOX-Engine";
        case PartType::MPEngine: return "MonoProp-Engine";
        case PartType::XenonEngine: return "Xenon-Engine";
        case PartType::SolidBooster: return "Solid Fuel Booster";
        default: return "Unknown Part Type";
    }
}

void Rocket::setRootPart(std::string_view partName) {
    for (auto& part : partCatalogue) {
        if (part.name == partName) {
            root = &part;
            return;
        }
    }
    throw std::runtime_error("Part not found in catalogue: " + std::string(partName));
}

Rocket::Rocket(const std::string& name) : name(name) { }

void Rocket::loadPartCatalogue(const std::filesystem::path& path) {
    loadPartCatalogueFromKSP(path, partCatalogue);

    for (const auto& part : partCatalogue) {
        std::cout << partTypeToString(part.type) << " \"" << part.name << "\"" ", Mass: " 
            << part.mass << " tons)" << "[" << part.attTop << part.attBottom << ']' << std::endl;
    }
}
