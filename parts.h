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
    bool hasResources() const {
        return liquidFuel > 0 || oxidizer > 0 || monoPropellant > 0 || solidFuel > 0 || xenonGas > 0;
    }
};

struct EngineISPInfo {
    std::vector<std::pair<double, double>> isp; // pairs of (atm pressure, ISP)
    double fuelConsumptionRate = 0.0f;
};

class PartProperty {
public:
    PartProperty() = delete;
    PartProperty(PartType part_type, 
         const std::string& parttitle, 
         double emptymass, 
         int att_top, 
         int att_bot, 
         float part_length,
         int maxCrew, 
         ResourceContainer res,
         double thrustkN,
         EngineISPInfo isp);

    const PartType type;
    const std::string title;
    const double mass = 0.0;
    const int attTop = 0;
    const int attBottom = 0;
    float length = 0.0f; // Distance between attachment nodes 
    
    // Resource capacities
    const int MaxCrew = 0;
    ResourceContainer resources;
    const double MaxThrustkN  = 0.0;
    EngineISPInfo ispCurve;

    int crewAmount = 0;

    double getMass(double fillPercent=1.0) const;

    void print() const;
};

class Part {
public:
    Part(const PartProperty* part_property) : part(part_property) { }
    void attachBelow(Part* part);
    void attachAbove(Part* part);

    const PartProperty* part;
    Part* below = nullptr; // The part directly attached below this part. Null if this is the bottommost part of a stage
    Part* above = nullptr;    // The part directly attached above this part. Null if this is the topmost part of a stage
};


#endif // PARTS_H
