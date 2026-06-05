#include "LoadKspParts.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include "kspConstants.h"

std::string trimString (const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
};

std::tuple<int, float> parseNodeSize(const std::string& line) {
    auto eq = line.find(" = ");
    if (eq == std::string::npos) return {0, 0.0f};
    std::string vals = trimString(line.substr(eq + 3));
    size_t pos = 0;

    // value 0: x — skip
    size_t end = vals.find(',', pos);
    if (end == std::string::npos) return {0, 0.0f};
    pos = end + 1;

    // value 1: y — read
    end = vals.find(',', pos);
    if (end == std::string::npos) return {0, 0.0f};
    float yPos = std::stof(trimString(vals.substr(pos, end - pos)));
    pos = end + 1;

    // values 2-4: z, dir_x, dir_y — skip
    for (int i = 0; i < 3; ++i) {
        end = vals.find(',', pos);
        if (end == std::string::npos) return {0, 0.0f};
        pos = end + 1;
    }
    // value 5: dir_z — skip (may be last if size omitted)
    end = vals.find(',', pos);
    if (end == std::string::npos) {
        return {-1, yPos};
    }
    pos = end + 1;

    // value 6: size
    end = vals.find(',', pos);
    int size = std::stoi(trimString(vals.substr(pos, end - pos)));

    return {size, yPos};
};

int bulkheadProfileToDefaultSize(const std::string& profiles) {
    // Use the first non-"srf" bulkhead profile to infer node size
    std::istringstream ss(profiles);
    std::string profile;
    while (std::getline(ss, profile, ',')) {
        profile = trimString(profile);
        if (profile == "size0") return 0;
        if (profile == "size1") return 1;
        if (profile == "size2" || profile == "mk2") return 2;
        if (profile == "size3" || profile == "mk3") return 3;
    }
    return 0;
}

std::tuple<float, float> parseISP(const std::string& line) {
    auto pos = line.find("key = ");
    if (pos == std::string::npos) return {0.0f, 0.0f};
    std::istringstream iss(trimString(line.substr(pos + 6)));
    float atm, isp;
    iss >> atm >> isp;
    return {atm, isp};
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

std::string getTitleValue(const std::string& line) {
    std::string raw = getStringValue(line);
    if (raw.empty()) return raw;

    auto commentPos = raw.find("//");
    if (commentPos != std::string::npos) {
        std::string comment = raw.substr(commentPos + 2);
        auto eqPos = comment.rfind("= ");
        if (eqPos != std::string::npos) {
            return trimString(comment.substr(eqPos + 2));
        }
    }

    return raw;
}

std::optional<KSPPart> loadFuelTankPart(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::nullopt;
    }

    KSPPart tank;
    ResourceType rtype = ResourceType::UNKNOWN;
    std::string bulkheadProfiles;

    std::string line;
    while (std::getline(file, line)) {
        // Simple
        if (line.find("title = ") != std::string::npos) {
            if (tank.title.empty()) {
                tank.title = getTitleValue(line);
            }
        }
        else if (line.find("mass = ") != std::string::npos) {
            tank.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            tank.attTop = attSize;
            tank.attTopY = yPos;
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            tank.attBottom = attSize;
            tank.attBottomY = yPos;
        }
        else if (line.find("bulkheadProfiles") != std::string::npos) {
            bulkheadProfiles = getStringValue(line);
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

    if (tank.attTop == -1) {
        tank.attTop = bulkheadProfileToDefaultSize(bulkheadProfiles);
    }
    if (tank.attBottom == -1) {
        tank.attBottom = bulkheadProfileToDefaultSize(bulkheadProfiles);
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
    std::string bulkheadProfiles;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("title = ") != std::string::npos) {
            if (engine.title.empty()) {
                engine.title = getTitleValue(line);
            }
        }
        else if (line.find("mass = ") != std::string::npos) {
            engine.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            engine.attTop = attSize;
            engine.attTopY = yPos;
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            engine.attBottom = attSize;
            engine.attBottomY = yPos;
        }
        else if (line.find("bulkheadProfiles") != std::string::npos) {
            bulkheadProfiles = getStringValue(line);
        }
        else if (line.find("maxThrust = ") != std::string::npos) {
            engine.VacThrust = getNumberInLine(line);
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
        else if (line.find("atmosphereCurve") != std::string::npos) {
            while (std::getline(file, line) && line.find("}") == std::string::npos) {
                if (line.find("key = ") != std::string::npos) {
                    auto [atm, isp] = parseISP(line);
                    engine.ispCurve.isp.emplace_back(atm, isp);
                }
            }
        }
    }

    if (engine.attTop == -1) {
        engine.attTop = bulkheadProfileToDefaultSize(bulkheadProfiles);
    }
    if (engine.attBottom == -1) {
        engine.attBottom = bulkheadProfileToDefaultSize(bulkheadProfiles);
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
    std::string bulkheadProfiles;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("title = ") != std::string::npos) {
            if (pod.title.empty()) {
                pod.title = getTitleValue(line);
            }
        }
        else if (line.find("mass = ") != std::string::npos) {
            pod.mass = getNumberInLine(line);
        }
        else if (line.find("node_stack_top") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            pod.attTop = attSize;
            pod.attTopY = yPos;
        }
        else if (line.find("node_stack_bottom") != std::string::npos) {
            auto [attSize, yPos] = parseNodeSize(line);
            pod.attBottom = attSize;
            pod.attBottomY = yPos;
        }
        else if (line.find("bulkheadProfiles") != std::string::npos) {
            bulkheadProfiles = getStringValue(line);
        }
        else if (line.find("CrewCapacity = ") != std::string::npos) {
            pod.crewCapacity = getNumberInLine(line);
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

    if (pod.attTop == -1) {
        pod.attTop = bulkheadProfileToDefaultSize(bulkheadProfiles);
    }
    if (pod.attBottom == -1) {
        pod.attBottom = bulkheadProfileToDefaultSize(bulkheadProfiles);
    }

    return pod;
}

void loadPartCatalogueFromKSP(const std::filesystem::path& ksp_path, std::vector<PartProperty>& partCatalogue) {
    auto partsFolder = ksp_path / "GameData" / "Squad" / "Parts";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "FuelTank")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto tankOpt = loadFuelTankPart(entry.path());
        if (tankOpt.has_value()) {
            auto tank = tankOpt.value();
            partCatalogue.emplace_back(tank.type,
                                       tank.title, 
                                       tank.mass, 
                                       tank.attTop,
                                       tank.attBottom,
                                       tank.attTopY - tank.attBottomY,
                                       0, // crew
                                       tank.resources,
                                       0.0, // thrust
                                       EngineISPInfo{}); // consumption
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Engine")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto engineOpt = loadEnginePart(entry.path());
        if (engineOpt.has_value()) {
            auto engine = engineOpt.value();
            partCatalogue.emplace_back(engine.type,
                                       engine.title,
                                       engine.mass,
                                       engine.attTop,
                                       engine.attBottom,
                                       engine.attTopY - engine.attBottomY,
                                       0, // crew
                                       engine.resources,
                                       engine.VacThrust,
                                       engine.ispCurve);
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Command")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg") { continue; }
        auto podOpt = loadCommandPodPart(entry.path());
        if (podOpt.has_value()) {
            auto pod = podOpt.value();

            partCatalogue.emplace_back(pod.type,
                                       pod.title,
                                       pod.mass, 
                                       pod.attTop,
                                       pod.attBottom,
                                       pod.attTopY - pod.attBottomY,
                                       pod.crewCapacity,
                                       pod.resources,
                                       0.0, // thrust
                                       EngineISPInfo{}); // ISP
        }
    }
}
