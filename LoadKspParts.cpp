#include "LoadKspParts.h"
#include <iostream>
#include <fstream>

std::string trimString (const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
};

int parseNodeSize(const std::string& line) {
    auto eq = line.find(" = ");
    if (eq == std::string::npos) return 0;
    std::string vals = trimString(line.substr(eq + 3));
    size_t pos = 0;
    for (int i = 0; i < 6; ++i) {
        pos = vals.find(',', pos);
        if (pos == std::string::npos) return 0;
        ++pos;
    }
    size_t end = vals.find(',', pos);
    return std::stoi(trimString(vals.substr(pos, end - pos)));
};

ResourceType parseResourceType(const std::string& resName) {
    if (resName == "MonoPropellant")  { return ResourceType::MP; }
    else if (resName == "LiquidFuel") { return ResourceType::LF; }
    else if (resName == "Oxidizer")   { return ResourceType::OX; }
    else if (resName == "SolidFuel")  { return ResourceType::SOLID; }
    else if (resName == "XenonGas")   { return ResourceType::XE; }
    return ResourceType::UNKNOWN;
}

double getNumberInLine(const std::string& line) {
    // Format key =  23.43
    auto pos = line.find('=') + 1;
    if (pos == std::string::npos) { return 0.0; }
    std::string numStr = trimString(line.substr(pos));
    try {
        return std::stod(numStr);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing number from line: " << line << " - " << e.what() << std::endl;
        return 0.0;
    }
}

std::string getStringValue(const std::string& line) {
    auto pos = line.find('=') + 1;
    if (pos == std::string::npos) { return ""; }
    return trimString(line.substr(pos));
}

std::optional<KSPPart> loadFuelTankPart(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::nullopt;
    }

    KSPPart tank;
    ResourceType rtype = ResourceType::UNKNOWN;

    std::string line;
    while (std::getline(file, line)) {
        // Simple
        if (line.find("name = ") != std::string::npos) {
             tank.name = getStringValue(line);
        }
        else if (line.find("mass = ") != std::string::npos) {
            tank.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            tank.attTop = parseNodeSize(line);
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            tank.attBottom = parseNodeSize(line);
        }
        else if (line.find("RESOURCE") != std::string::npos) {
            while (std::getline(file, line) && line.find("}") == std::string::npos) {
                if (line.find("name = ") != std::string::npos) {
                    std::string resourceName = getStringValue(line);
                    if (resourceName == "LiquidFuel") { 
                        rtype = LF; 
                        if (tank.type == PartType::UNKNOWN) {
                            tank.type = PartType::LFTank;
                        }
                    }
                    else if (resourceName == "Oxidizer") { 
                        rtype = OX; 
                        tank.type = PartType::LFOXTank;
                    }
                    else if (resourceName == "MonoPropellant") { 
                        rtype = MP; 
                        tank.type = PartType::MPTank;
                    }
                    else if (resourceName == "XenonGas") { 
                        rtype = XE;
                        tank.type = PartType::XenonTank;
                    }
                }
                else if (line.find("maxAmount = ") != std::string::npos) {
                    double amount = getNumberInLine(line);
                    switch (rtype) {
                        case LF:    tank.resources.liquidFuel     = amount; break;
                        case OX:    tank.resources.oxidizer       = amount; break;
                        case MP:    tank.resources.monoPropellant = amount; break;
                        case SOLID: tank.resources.solidFuel      = amount; break;
                        case XE:    tank.resources.xenonGas       = amount; break;
                        default: break;
                    }
                }
            }
        }
    }
    return tank;
}

std::optional<Engine> loadEnginePart(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::nullopt;
    }

    Engine engine;
    ResourceType rtype = ResourceType::UNKNOWN;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("name = ") != std::string::npos) {
            if (engine.name.empty()) {
                engine.name = getStringValue(line);
            }
        }
        else if (line.find("mass = ") != std::string::npos) {
            engine.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            engine.attTop = parseNodeSize(line);
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            engine.attBottom = parseNodeSize(line);
        }
        else if (line.find("maxThrust = ") != std::string::npos) {
            std::string thrustStr = line.substr(line.find("maxThrust = ") + 12);
            engine.Thrust = getNumberInLine(line);
        }
        else if (line.find("PROPELLANT") != std::string::npos) {
            while (std::getline(file, line) && line.find("}") == std::string::npos) {
                if (line.find("name = ") != std::string::npos) {
                    ResourceType res = parseResourceType(getStringValue(line));
                    switch (res) {
                        case LF:    engine.type = PartType::LFEngine; break;
                        case OX:    engine.type = PartType::LOXEngine; break;
                        case MP:    engine.type = PartType::MPEngine; break;
                        case SOLID: engine.type = PartType::SolidBooster; break;
                        case XE:    engine.type = PartType::XenonEngine; break;
                        default: break;
                    }
                }
            }
        }
        else if (line.find("RESOURCE") != std::string::npos) {
            while (std::getline(file, line) && line.find("}") == std::string::npos) {
                if (line.find("name = ") != std::string::npos) {
                    rtype = parseResourceType(getStringValue(line));
                }
                else if (line.find("amount = ") != std::string::npos) {
                    double amount = getNumberInLine(line);
                    switch (rtype) {
                        case LF:    engine.resources.liquidFuel     = amount; break;
                        case OX:    engine.resources.oxidizer       = amount; break;
                        case MP:    engine.resources.monoPropellant = amount; break;
                        case SOLID: engine.resources.solidFuel      = amount; break;
                        case XE:    engine.resources.xenonGas       = amount; break;
                        default: break;
                    }
                }
            }
        }
    }
    return engine;
}

std::optional<CmdPod> loadCommandPodPart(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::nullopt;
    }
    CmdPod pod;
    ResourceType rtype = ResourceType::UNKNOWN;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("name = ") != std::string::npos) {
            if (pod.name.empty()) {
                pod.name = getStringValue(line);
            }
        }
        else if (line.find("mass = ") != std::string::npos) {
            pod.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            pod.attTop = parseNodeSize(line);
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            pod.attBottom = parseNodeSize(line);
        }
        else if (line.find("CrewCapacity = ") != std::string::npos) {
            std::string crewStr = line.substr(line.find("CrewCapacity = ") + 15);
            pod.crewCapacity = std::stoi(crewStr);
            if (pod.crewCapacity > 0) {
                pod.type = PartType::CrewedCommandPod;
            }
            else {
                pod.type = PartType::DronePod;
            }
        }
        else if (line.find("RESOURCE") != std::string::npos) {
            rtype = ResourceType::UNKNOWN;
            while (std::getline(file, line) && line.find("}") == std::string::npos) {
                if (line.find("name = ") != std::string::npos) {
                    rtype = parseResourceType(getStringValue(line));
                }
                else if (line.find("maxAmount = ") != std::string::npos) {
                    double amount = getNumberInLine(line);
                    switch (rtype) {
                        case ResourceType::MP:    pod.resources.monoPropellant = amount; break;
                        case ResourceType::LF:    pod.resources.liquidFuel     = amount; break;
                        case ResourceType::OX:    pod.resources.oxidizer       = amount; break;
                        case ResourceType::SOLID: pod.resources.solidFuel      = amount; break;
                        case ResourceType::XE:    pod.resources.xenonGas       = amount; break;
                        default: break;
                    }
                }
            }
        }
    }
    return pod;
}

void loadPartCatalogueFromKSP(const std::filesystem::path& ksp_path, std::vector<Part>& partCatalogue) {
    auto partsFolder = ksp_path / "GameData" / "Squad" / "Parts";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "FuelTank")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto tankOpt = loadFuelTankPart(entry.path());
        if (tankOpt.has_value()) {
            std::cout << "Loaded fuel tank part from " << entry.path() << std::endl;
            auto tank = tankOpt.value();
            partCatalogue.emplace_back(tank.type,
                                       tank.name, 
                                       tank.mass, 
                                       tank.attTop,
                                       tank.attBottom,
                                       tank.attTopY - tank.attBottomY,
                                       0, // crew
                                       tank.resources,
                                       0.0, // thrust
                                       0.0); // consumption
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Engine")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto engineOpt = loadEnginePart(entry.path());
        if (engineOpt.has_value()) {
            std::cout << "Loaded engine part from " << entry.path() << std::endl;
            auto engine = engineOpt.value();
            partCatalogue.emplace_back(engine.type,
                                       engine.name,
                                       engine.mass,
                                       engine.attTop,
                                       engine.attBottom,
                                       engine.attTopY - engine.attBottomY,
                                       0, // crew
                                       engine.resources,
                                       engine.Thrust,
                                       0.0); // fuel consumption (can be calculated later)
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Command")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto podOpt = loadCommandPodPart(entry.path());
        if (podOpt.has_value()) {
            std::cout << "Loaded command pod part from " << entry.path() << std::endl;
            auto pod = podOpt.value();

            partCatalogue.emplace_back(pod.type,
                                       pod.name,
                                       pod.mass, 
                                       pod.attTop,
                                       pod.attBottom,
                                       pod.attTopY - pod.attBottomY,
                                       pod.crewCapacity,
                                       pod.resources,
                                       0.0, // thrust
                                       0.0); // fuelConsumption
        }
    }
}
