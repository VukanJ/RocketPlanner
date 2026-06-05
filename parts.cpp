#include "parts.h"

#include <iostream>
#include "kspConstants.h"

Part::Part(PartType part_type, 
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
    ispCurve(isp_curve) 
{ 
    // If engine, compute consumption rate based on max thrust and ISP at vacuum and fuel density
    if (isEngine(part_type)) {
        double ispVac = 0.0;
        for (const auto& [pressure, isp] : ispCurve.isp) {
            if (pressure == 0.0) { // Vacuum ISP
                ispVac = isp;
                break;
            }
        }
        if (ispVac > 0.0) {
            switch (part_type) {
                case PartType::LFEngine:
                    ispCurve.fuelConsumptionRate = MaxThrustkN / (ispVac * Constants::g0_kerbin * Constants::LiquidFuelDensity);
                    break;
                case PartType::LOXEngine:
                    {
                        // Its always the same ratio
                        float meanDensity = (Constants::LiquidFuelDensity * 0.9 + Constants::OxidizerDensity * 1.1) / 2.0;
                        ispCurve.fuelConsumptionRate = MaxThrustkN / (ispVac * Constants::g0_kerbin * meanDensity);
                    }
                    break;
                case PartType::MPEngine:
                    ispCurve.fuelConsumptionRate = MaxThrustkN / (ispVac * Constants::g0_kerbin * Constants::MonoPropellantDensity);
                    break;
                case PartType::XenonEngine:
                    ispCurve.fuelConsumptionRate = MaxThrustkN / (ispVac * Constants::g0_kerbin * Constants::XenonGasDensity);
                    break;
                case PartType::SolidBooster:
                    ispCurve.fuelConsumptionRate = MaxThrustkN / (ispVac * Constants::g0_kerbin * Constants::SolidFuelDensity);
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
        default: return "Unknown Part Type";
    }
}

void Part::print() const {
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

    if (!ispCurve.isp.empty()) {
        std::cout << "\tFuel Consumption Rate: " << ispCurve.fuelConsumptionRate << " L/s\n";
        std::cout << "\tISP Curve:\n";
        for (const auto& [pressure, isp] : ispCurve.isp) {
            std::cout << "\t  " << pressure << " atm -> " << isp << " s\n";
        }
    }
}

double Part::getMass(double fillPercent) const {
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
