#include "rocket.h"

#include "helper.h"
#include "RocketSolver.h"

#include <iostream>
#include <cmath>

Rocket::Rocket(const std::filesystem::path& kspPath, const std::string& rocket_name) 
    : name(rocket_name) 
{
    // Load part catalogue from KSP installation
    partCatalogue.loadPartCatalogue(kspPath);
}

void Rocket::setRootPart(std::string_view partName) {
    for (auto& part : partCatalogue.allParts) {
        if (part.title == partName) {
            root = &part;
            return;
        }
    }
    throw std::runtime_error("Part not found in catalogue: " + std::string(partName));
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

void Rocket::construct(double targetDeltaV, double payloadMass, double minTWR, double g0, double seaLevelAtm) {
    if (root == nullptr) {
        throw std::runtime_error("Root part not set. Call setRootPart() before constructing the rocket.");
    }

    RocketSolver solver(partCatalogue.all_engines);
    solver.solve(targetDeltaV, payloadMass, minTWR, g0, seaLevelAtm);

    //// Initialize default stage
    //stages.emplace_back();
    //stages.back().setTop(root);
    //
    //// Start optimization
    ////Stage& currentStage = stages.back();
    ////Part* p = nullptr;
    ////p = currentStage.attachBelow(currentStage.Top, tanks_LOX.back());
    ////p = currentStage.attachBelow(p, engines_LOX.back());
    //
    //// First, assume single stage
    //// Compute required mass ratio using Tsiolkovsky's equation, then add fuel tanks and engines until we meet the requirements
    //// Tsiolkovsky: Δv = Isp * g0 * ln((m0 + payload)/(mf + payload))
    //// Rearranging: m0 = mf * exp(Δv / (Isp * g0))
    //
    //std::vector<std::tuple<int, const PartProperty*, float, float>> viableEngineOptions; // (number of engines, engine pointer)
    //for (const auto& engine : partCatalogue.engines_LF) {
    //    for (int i = 1; i <= 64; ++i) { // Try multiple engines
    //        double thrust = engine->MaxThrustkN * i;
    //        double engine_mass = engine->getMass(1.0) * i;
    //        double R = std::exp(targetDeltaV / (engine->enginePerf.vacuumISP * Constants::g0_kerbin));
    //        if ((R - 1.0) / 8.0 >= 1.0) {
    //            // Mass ratio exceeds what the tanks can support (dry mass becomes greater than or equal to fuel mass), so skip this engine configuration
    //            continue;
    //        }
    //        // LF/OX tank dry mass = fuel_mass / 8  (full/dry = 9:1 ratio from stock KSP parts)
    //        double dry_base = payloadMass + engine_mass;
    //        double mfinal = dry_base / (1.0 - (R - 1.0) / 8.0);  // Closed form solution for final mass. The more fuel, the more dry mass. This breaks the loop.
    //        double mfull = mfinal * R;
    //        double TWR_start = thrust / (mfull * g0);
    //        //double TWR_end = thrust / (mfinal * g0);
    //        if (TWR_start >= minTWR) {
    //            viableEngineOptions.emplace_back(i, engine, mfinal-dry_base, TWR_start);
    //            break;
    //        }
    //    }
    //}
    //
    //for (const auto& [numEngines, engine, fuel, TWR] : viableEngineOptions) {
    //    println<BLUE>("Viable engine option:", numEngines, "x", engine->title, ", Fuel mass", fuel, "minTWR:", TWR);
    //}
}
