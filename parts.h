#ifndef PARTS_H
#define PARTS_H

#include <vector>
#include <string>

// Weight in tons
constexpr double AstronautMass = 0.1;

enum class PartType {
    CrewedCommandPod,
    DronePod,
    LFTank,
    LFOXTank,
    MPTank,
    XenonTank,
    LFEngine,
    LOXEngine,
    MPEngine,
    XenonEngine,
    SolidBooster,
    UNKNOWN
};

class Part {
public:
    Part() = delete;
    Part(PartType type, const std::string& name, double mass, int attsize0, int attsize1,
         int maxCrew, double maxLiquidFuel, double maxOxidizer, double maxMonoPropellant,
         double thrustkN, double fuelConsumption) :
        type(type), name(name), mass(mass), attsize0(attsize0), attsize1(attsize1),
        MaxCrew(maxCrew), MaxLiquidFuel(maxLiquidFuel), MaxOxidizer(maxOxidizer),
        MaxMonoPropellant(maxMonoPropellant), ThrustkN(thrustkN), FuelConsumption(fuelConsumption) { }

    const PartType type;
    const std::string name;
    const double mass = 0.0;
    const int attsize0 = 0;
    const int attsize1 = 0;
    
    // Resource capacities
    const int MaxCrew = 0;
    const double MaxLiquidFuel = 0.0;
    const double MaxOxidizer = 0.0;
    const double MaxMonoPropellant = 0.0;

    double liquidFuelLevel = 1.0;
    double oxidizerLevel = 1.0;
    double monoPropellantLevel = 1.0;
    int crewAmount = 0;

    const double ThrustkN  = 0.0;
    const double FuelConsumption = 0.0; // kg per second

    double getMass() const {
        return mass;
    }

    std::vector<Part*> attachedParts;
};


#endif // PARTS_H
