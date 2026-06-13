#include "parts.h"

#include <iostream>
#include "LoadKspParts.h"
#include "kspConstants.h"
#include "helper.h"

PartProperty::PartProperty(PartType part_type, 
     const std::string& parttitle, 
     double emptymass, 
     int att_top, 
     int att_bot, 
     float part_length,
     int maxCrew, 
     ResourceContainer res,
     double thrustkN, 
     EngineISPInfo isp_curve) :
    type(part_type), 
    title(parttitle), 
    mass(emptymass), 
    attTop(att_top), 
    attBottom(att_bot), 
    length(part_length),
    MaxCrew(maxCrew), 
    resources(res), 
    MaxThrustkN(thrustkN), 
    enginePerf(isp_curve) 
{ 
    // If engine, compute consumption rate based on max thrust and ISP at vacuum and fuel density
    if (isEngine(part_type)) {
        for (const auto& [pressure, isp] : enginePerf.isp) {
            if (pressure == 0.0) { // Vacuum ISP
                enginePerf.vacuumISP = isp;
                break;
            }
        }
        if (enginePerf.vacuumISP > 0.0) {
            switch (part_type) {
                case PartType::LFEngine:
                    enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * Constants::LiquidFuelDensity);
                    break;
                case PartType::LOXEngine:
                    {
                        // Its always the same ratio
                        float meanDensity = (Constants::LiquidFuelDensity * 0.9 + Constants::OxidizerDensity * 1.1) / 2.0;
                        enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * meanDensity);
                    }
                    break;
                case PartType::MPEngine:
                    enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * Constants::MonoPropellantDensity);
                    break;
                case PartType::XenonEngine:
                    enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * Constants::XenonGasDensity);
                    break;
                case PartType::SolidBooster:
                    enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * Constants::SolidFuelDensity);
                    break;
                case PartType::JetEngine:
                    enginePerf.fuelConsumptionRate_UPS = MaxThrustkN / (enginePerf.vacuumISP * Constants::g0_kerbin * Constants::LiquidFuelDensity);
                    break;
                default: break;
            }
        }
    }
}

std::string partTypeToString(PartType type) {
    switch (type) {
        case PartType::CrewedCommandPod: return "Crewed Command Pod";
        case PartType::DronePod:         return "DronePod";
        case PartType::LFTank:           return "LF-Tank";
        case PartType::LFOXTank:         return "LOX-Tank";
        case PartType::MPTank:           return "MonoProp-Tank";
        case PartType::XenonTank:        return "Xenon-Tank";
        case PartType::LFEngine:         return "LF-Engine";
        case PartType::LOXEngine:        return "LOX-Engine";
        case PartType::MPEngine:         return "MonoProp-Engine";
        case PartType::XenonEngine:      return "Xenon-Engine";
        case PartType::SolidBooster:     return "Solid Fuel Booster";
        case PartType::JetEngine:        return "Jet Engine";
        default: return "Unknown Part Type";
    }
}

void PartProperty::print() const {
    std::cout << "Part: " << title << "\n";
    std::cout << "\tType: " << partTypeToString(type) << "\n";
    std::cout << "\tMass: " << mass << " tons\n";
    if (resources.hasResources()) {
        std::cout << "\tMass (Full): " << getMass(1.0) << " tons\n";
    }
    std::cout << "\tAttachment: " << attBottom << " -> " << attTop << "\n";
    std::cout << "\tLength: " << length << " m\n";
    if (MaxCrew > 0) {
        std::cout << "\tCrew Capacity: " << MaxCrew << "\n";
    }
    if (MaxThrustkN > 0.0) {
        std::cout << "\tThrust(Vac): " << MaxThrustkN << " kN\n";
    }
    if (resources.hasResources()) {
        std::cout << "\tResources:\n";
        if (resources.liquidFuel > 0) {
            std::cout << "\t\tLiquid Fuel: " << resources.liquidFuel << "L\n";
        }
        if (resources.oxidizer > 0) {
            std::cout << "\t\tOxidizer: " << resources.oxidizer << "L\n";
        }
        if (resources.monoPropellant > 0) {
            std::cout << "\t\tMonoPropellant: " << resources.monoPropellant << "L\n";
        }
        if (resources.solidFuel > 0) {
            std::cout << "\t\tSolid Fuel: " << resources.solidFuel << "L\n";
        }
        if (resources.xenonGas > 0) {
            std::cout << "\t\tXenon Gas: " << resources.xenonGas << "L\n";
        }
    }

    if (!enginePerf.isp.empty()) {
        std::cout << "\tFuel Consumption Rate: " << enginePerf.fuelConsumptionRate_UPS << " L/s\n";
        std::cout << "\tISP Curve:\n";
        for (const auto& [pressure, isp] : enginePerf.isp) {
            std::cout << "\t  " << pressure << " atm -> " << isp << " s\n";
        }
    }
}

double PartProperty::getMass(double fillPercent) const {
    if (fillPercent < 0.0 || fillPercent > 1.0) {
        throw std::invalid_argument("Fill percent must be between 0 and 1");
    }

    if (resources.hasResources()) {
        double totalMass = mass;
        totalMass += resources.liquidFuel * Constants::LiquidFuelDensity;
        totalMass += resources.oxidizer * Constants::OxidizerDensity;
        totalMass += resources.monoPropellant * Constants::MonoPropellantDensity;
        totalMass += resources.solidFuel * Constants::SolidFuelDensity;
        totalMass += resources.xenonGas * Constants::XenonGasDensity;

        return mass + fillPercent * (totalMass - mass);
    }
    else {
        return mass;
    }
}

void Part::attachBelow(Part* att) {
    // Check if attachment is valid
    below = att;
    att->above = this;
}

void Part::attachAbove(Part* att) {
    // Check if attachment is valid
    above = att;
    att->below = this;
}

double ResourceContainer::getUsableFuelUnits(const Part* engine) const {
    switch (engine->part->type) {
        case PartType::LFEngine:
            return liquidFuel;
        case PartType::LOXEngine:
            return liquidFuel + oxidizer;
        case PartType::MPEngine:
            return monoPropellant;
        case PartType::XenonEngine:
            return xenonGas;
        case PartType::SolidBooster:
            return solidFuel;
        case PartType::JetEngine:
            return liquidFuel;
        default:
            return 0.0;
    }
}

double PartProperty::usedFuelDensity() const {
    switch (type) {
        case PartType::LFEngine:
            return Constants::LiquidFuelDensity;
        case PartType::LOXEngine:
            return 0.5 * (Constants::OxidizerDensity * 1.1 + Constants::LiquidFuelDensity * 0.9);
        case PartType::MPEngine:
            return Constants::MonoPropellantDensity;
        case PartType::XenonEngine:
            return Constants::XenonGasDensity;
        case PartType::SolidBooster:
            return Constants::SolidFuelDensity;
        case PartType::JetEngine:
            return Constants::LiquidFuelDensity;
        default:
            return 0.0;
    }
}

void PartCatalogue::loadPartCatalogue(const std::filesystem::path& path) {
    loadPartCatalogueFromKSP(path, allParts);
    for (const auto& part : allParts) {
        //part.print();
        switch (part.type) {
            case PartType::MPTank:           tanks_MP.push_back(&part); break;
            case PartType::LFTank:           tanks_LF.push_back(&part); break;
            case PartType::LFOXTank:         tanks_LOX.push_back(&part); break;
            case PartType::LFEngine:         engines_LF.push_back(&part); break;
            case PartType::LOXEngine:        engines_LOX.push_back(&part); break;
            case PartType::SolidBooster:     engines_Booster.push_back(&part); break;
            case PartType::JetEngine:        engines_Jet.push_back(&part); break;
            case PartType::CrewedCommandPod: command_modules.push_back(&part); break;
            case PartType::DronePod:         command_modules.push_back(&part); break;
            default: break;
        }
    }

    all_engines.insert(all_engines.end(), engines_LF.begin(), engines_LF.end());
    all_engines.insert(all_engines.end(), engines_LOX.begin(), engines_LOX.end());
    all_engines.insert(all_engines.end(), engines_Booster.begin(), engines_Booster.end());
}
