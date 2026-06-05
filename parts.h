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

struct ResourceContainer {
    double liquidFuel = 0.0;
    double oxidizer = 0.0;
    double monoPropellant = 0.0;
    double solidFuel = 0.0;
    double xenonGas = 0.0;
};

class Part {
public:
    Part() = delete;
    Part(PartType part_type, 
         const std::string& partname, 
         double emptymass, 
         int att_top, 
         int att_bot, 
         float part_length,
         int maxCrew, 
         ResourceContainer res,
         double thrustkN, 
         double fuelConsumption) :
        type(part_type), name(partname), mass(emptymass), attTop(att_top), attBottom(att_bot), length(part_length),
        MaxCrew(maxCrew), resources(res), ThrustkN(thrustkN), FuelConsumption(fuelConsumption) { }

    const PartType type;
    const std::string name;
    const double mass = 0.0;
    const int attTop = 0;
    const int attBottom = 0;
    float length = 0.0f; // Distance between attachment nodes 
    
    // Resource capacities
    const int MaxCrew = 0;
    ResourceContainer resources;

    int crewAmount = 0;

    const double ThrustkN  = 0.0;
    const double FuelConsumption = 0.0; // kg per second

    double getMass() const;
};


#endif // PARTS_H
