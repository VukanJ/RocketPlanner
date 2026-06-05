#include "stage.h"
#include "helper.h"

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

        float totalFuel = 0; // Total usable fuel for this engine
        float burnTimeSecond = 0;
    };

    std::vector<BurnStack> burnStacks;
    for (const auto& engine : engines) {
        // Identify fuel tanks for this engine
        BurnStack burnstack;
        Part* current = engine;
        burnstack.engine = engine;
        while (current != nullptr) {
            if (current->part->resources.hasResources()) {
                burnstack.fuelTanks.push_back(current);
            }
            current = current->above;
            if (current == Top) {
                break; // Don't go beyond the top of the stage
            }
        }
    }
    // Do all burn stacks burn the for the same duration?
    // If not, then the stage will carry dead weight after shorter burn stacks runs out of fuel
    // If it does, then, DeltaV can be computed in a single step

    // Compute total usable fuel
    for (auto& burnstack : burnStacks) {
        for (const auto& tank : burnstack.fuelTanks) {
            // For now, assume all fuel in the tank is usable. In reality, some fuel may be trapped and unusable depending on the configuration of the stage and the position of the tanks
            burnstack.totalFuel += tank->part->resources.liquidFuel;
            burnstack.totalFuel += tank->part->resources.oxidizer;
            burnstack.totalFuel += tank->part->resources.monoPropellant;
            burnstack.totalFuel += tank->part->resources.solidFuel;
            burnstack.totalFuel += tank->part->resources.xenonGas;
        }
    }
    for (auto& burnstack : burnStacks) {
        burnstack.burnTimeSecond = burnstack.totalFuel / burnstack.engine->part->ispCurve.fuelConsumptionRate;
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
