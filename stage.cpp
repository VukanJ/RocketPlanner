#include "stage.h"
#include "helper.h"

#include <iostream>
#include <cmath>
#include <algorithm>

bool Stage::hasPropulsion() const {
    return std::any_of(all_parts.begin(), all_parts.end(), [](const Part& part) {
        return isEngine(part.part->type);
    });
}

double Stage::DeltaV() const {
    if (!hasPropulsion()) {
        return 0.0;
    }

    // Iterate through all engines, identify their fuel tanks and compute deltaV contribution using Tsiolkovsky's formula

    std::vector<Part*> engines;
    for (const auto& part : all_parts) {
        if (isEngine(part.part->type)) {
            engines.push_back(const_cast<Part*>(&part));
        }
    }

    struct BurnStack {
        // Engine and associated fuel tanks that feed it
        Part* engine;
        std::vector<Part*> fuelTanks;

        float usableFuelUnits = 0; // Total usable fuel for this engine
        float burnTimeSeconds = 0;
    };

    std::vector<BurnStack> burnStacks;
    for (const auto& engine : engines) {
        // Identify fuel tanks for this engine
        BurnStack burnstack;
        burnstack.engine = engine;
        Part* current = engine;
        while (current != nullptr) {
            if (current->part->resources.hasResources()) {
                burnstack.fuelTanks.push_back(current);
            }
            current = current->above;
            if (current == Top) {
                break; // Don't go beyond the top of the stage
            }
        }
        burnStacks.push_back(burnstack);
    }

    // Do all burn stacks burn for the same duration?
    // If not, then the stage will carry dead weight after shorter burn stacks runs out of fuel
    // If it does, then, DeltaV can be computed in a single step

    bool sameBurnTime = true;
    // Compute total usable fuel
    for (auto& bst : burnStacks) {
        for (const auto& tank : bst.fuelTanks) {
            bst.usableFuelUnits += tank->part->resources.getUsableFuelUnits(bst.engine);
        }
    }
    // Compute burn time for each burn stack
    for (auto& bst : burnStacks) {
        bst.burnTimeSeconds = bst.usableFuelUnits / bst.engine->part->enginePerf.fuelConsumptionRate_UPS;
    }

    float burnTime = burnStacks[0].burnTimeSeconds;
    for (const auto& bst : burnStacks) {
        if (std::abs(bst.burnTimeSeconds - burnTime) > 0.5) { // 0.5 seconds is fine...
            sameBurnTime = false;
            break;
        }
    }

    if (sameBurnTime) {
        // Compute total mass and effective ISP for the stage
        double totalStageMass = TotalMass();
        double emptyStageMass = totalStageMass;
        double effectiveISP = 0.0;
        double denom = 0.0;
        for (const auto& bst : burnStacks) {
            effectiveISP += bst.engine->part->MaxThrustkN;
            denom += bst.engine->part->MaxThrustkN / bst.engine->part->enginePerf.vacuumISP;

            for (const auto& tank : bst.fuelTanks) {
                emptyStageMass -= tank->part->resources.getUsableFuelUnits(bst.engine) * bst.engine->part->usedFuelDensity();
            }
        }
        effectiveISP = effectiveISP / denom; // Weighted harmonic mean of ISPs
        double deltaV = effectiveISP * Constants::g0_kerbin * std::log(totalStageMass / emptyStageMass);

        return deltaV;
    }
    else {
        throw std::runtime_error("DeltaV calculation for asynchronously depleting fuel tanks is not implemented yet");
    }

    return 42;
}

double Stage::TotalMass() const {
    double total = 0;
    for (const auto& part : all_parts) {
        total += part.part->getMass(1.0);
    }
    return total;
}

void Stage::setTop(const PartProperty* top) {
    all_parts.emplace_back(top);
    Top = &all_parts.back();
}

Part* Stage::attachAbove(Part* part, const PartProperty* attach) {
    all_parts.emplace_back(attach);
    Part* newPart = &all_parts.back();
    part->attachAbove(newPart);
    return newPart;
}

Part* Stage::attachBelow(Part* part, const PartProperty* attach) {
    all_parts.emplace_back(attach);
    Part* newPart = &all_parts.back();
    part->attachBelow(newPart);
    return newPart;
}
