#include "rocket.h"

#include "LoadKspParts.h"
#include "helper.h"

#include <iostream>
#include <cmath>

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

void Rocket::printConfig() const {
    std::cout << "Rocket: " << name << "\n";
    std::cout << "Stages: " << stages.size() << "\n";
    for (size_t i = 0; i < stages.size(); ++i) {
        std::cout << "Stage " << i+1 << ":\n";
        for (const auto& part : stages[i].all_parts) {
            std::cout << "\t" << part.part->title << "\n";
        }
    }
}

void Rocket::construct(double targetDeltaV, double payloadMass, double minTWR, double g0) {
    if (root == nullptr) {
        throw std::runtime_error("Root part not set. Call setRootPart() before constructing the rocket.");
    }

    // Initialize default stage
    stages.emplace_back();
    stages.back().setTop(root);

    // Start optimization
    //Stage& currentStage = stages.back();
    //Part* p = nullptr;
    //p = currentStage.attachBelow(currentStage.Top, tanks_LOX.back());
    //p = currentStage.attachBelow(p, engines_LOX.back());
    
    // First, assume single stage
    // Compute required mass ratio using Tsiolkovsky's equation, then add fuel tanks and engines until we meet the requirements
    // Tsiolkovsky: Δv = Isp * g0 * ln((m0 + payload)/(mf + payload))
    // Rearranging: m0 = mf * exp(Δv / (Isp * g0))
    for (const auto& engine : engines_LOX) {
        for (int i = 1; i < 8; ++i) { // Try 1 to 4 engines
            double thrust = engine->MaxThrustkN * i;
            double engine_mass = engine->getMass(1.0) * i;

            double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * Constants::g0_kerbin));
            if ((R - 1.0) / 8.0 >= 1.0) {
                println<RED>("Stage cannot achieve ", targetDeltaV, "m/s with engine:", engine->title, "(mass ratio exceeds tank dry mass limit)");
                continue;
            }
            // LF/OX tank dry mass = fuel_mass / 8  (full/dry = 9:1 ratio from stock KSP parts)
            double dry_base = payloadMass + engine_mass;
            double mfinal = dry_base / (1.0 - (R - 1.0) / 8.0);  // Closed form solution for final mass. The more fuel, the more dry mass, so it would be a loop.
            double mfull = mfinal * R;
            double TWR_start = thrust / (mfull * g0);
            double TWR_end = thrust / (mfinal * g0);
            if (TWR_start >= minTWR) {
                println<BLUE>(i, "x Engine:", engine->title, ". Required Fuel Mass: ", mfull - mfinal,  " tons, TWR: ",  TWR_start,  "(",  TWR_end,  ")");
                break;
            }
            else {
                println<RED>("XXX Engine:", engine->title, "does not meet TWR requirement. TWR: ", TWR_start, "(", TWR_end, ")");
            }
        }
    }
}
