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
        //part.print();
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

void Rocket::print() const {
    std::cout << "Rocket: " << name << "\n";
    std::cout << "Stages: " << stages.size() << "\n";
    for (size_t i = 0; i < stages.size(); ++i) {
        std::cout << "Stage " << i+1 << ":\n";
        for (const auto& part : stages[i].all_parts) {
            std::cout << "\t" << part.part->title << "\n";
        }
    }
}

void Rocket::construct(double targetDeltaV, double payloadMass, double minTWR) {
    if (root == nullptr) {
        throw std::runtime_error("Root part not set. Call setRootPart() before constructing the rocket.");
    }

    // Initialize default stage
    stages.emplace_back();
    stages.back().setTop(root);

    // Start optimization
    Stage& currentStage = stages.back();
    Part* p = nullptr;
    p = currentStage.attachBelow(currentStage.Top, tanks_LOX.back());
    p = currentStage.attachBelow(p, engines_LOX.back());

    std::cout << stages.back().DeltaV() << " m/s\n";
}
