#ifndef LOADKSPPARTS_H
#define LOADKSPPARTS_H

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>

#include "parts.h"

void loadPartCatalogueFromKSP(const std::filesystem::path& ksp_path, std::vector<Part>& partCatalogue) {
    auto trimString = [](const std::string& str) -> std::string {
        size_t start = str.find_first_not_of(" \t");
        size_t end = str.find_last_not_of(" \t");
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    };

    auto partsFolder = ksp_path / "GameData" / "Squad" / "Parts";

    enum Resource { LF, OX, MP, UNKNOWN } rtype;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "FuelTank")) {
        struct Tank {
            std::string name;
            PartType type = PartType::UNKNOWN;
            double mass = 0.0;
            double maxLF = 0.0;
            double maxOX = 0.0;
            double maxMP = 0.0;
            double LF = 0.0;
            double OX = 0.0;
            double MP = 0.0;
        };
        Tank tank;
        if (entry.is_regular_file() && entry.path().extension() == ".cfg") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
                continue;
            }

            std::string line;
            PartType tankType = PartType::UNKNOWN;
            while (std::getline(file, line)) {
                // Simple
                if (line.find("name = ") != std::string::npos) {
                    std::string partName = trimString(line.substr(line.find("name = ") + 7));
                    tank.name = partName;
                }
                else if (line.find("mass = ") != std::string::npos) {
                    std::string massStr = line.substr(line.find("mass = ") + 7);
                    tank.mass = std::stod(massStr);
                }
                else if (line.find("RESOURCE") != std::string::npos) {
                    while (std::getline(file, line) && line.find("}") == std::string::npos) {
                        if (line.find("name = ") != std::string::npos) {
                            std::string resourceName = trimString(line.substr(line.find("name = ") + 7));
                            if (resourceName == "LiquidFuel") { 
                                rtype = LF; 
                                if (tankType == PartType::UNKNOWN) {
                                    tankType = PartType::LFTank;
                                }
                            }
                            else if (resourceName == "Oxidizer") { 
                                rtype = OX; 
                                tankType = PartType::LFOXTank;
                            }
                            else if (resourceName == "MonoPropellant") { 
                                rtype = MP; 
                                tankType = PartType::MPTank;
                            }
                            else if (resourceName == "XenonGas") { 
                                rtype = MP; 
                                tankType = PartType::XenonTank;
                            }
                        }
                        else if (line.find("amount = ") != std::string::npos) {
                            std::string amountStr = line.substr(line.find("amount = ") + 9);
                            switch (rtype) {
                                case LF: tank.LF = std::stod(amountStr); break;
                                case OX: tank.OX = std::stod(amountStr); break;
                                case MP: tank.MP = std::stod(amountStr); break;
                                default: break;
                            }
                        }
                        else if (line.find("maxAmount = ") != std::string::npos) {
                            std::string amountStr = line.substr(line.find("maxAmount = ") + 12);
                            switch (rtype) {
                                case LF: tank.maxLF = std::stod(amountStr); break;
                                case OX: tank.maxOX = std::stod(amountStr); break;
                                case MP: tank.maxMP = std::stod(amountStr); break;
                                default: break;
                            }
                        }
                    }
                }
            }
            partCatalogue.emplace_back(tankType,
                                       tank.name, 
                                       tank.mass, 
                                       0,
                                       0, 
                                       0, // crew
                                       tank.maxLF, 
                                       tank.maxOX, 
                                       tank.maxMP, 
                                       0.0, 
                                       0.0);
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Engine")) {
        struct Engine {
            std::string name;
            double mass = 0.0;
            double Thrust = 0.0;
            };
        Engine engine;
        if (entry.is_regular_file() && entry.path().extension() == ".cfg") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
                continue;
            }
            std::string line;
            PartType engineType = PartType::UNKNOWN;
            while (std::getline(file, line)) {
                if (line.find("name = ") != std::string::npos) {
                    if (engine.name.empty()) {
                        engine.name = trimString(line.substr(line.find("name = ") + 7));
                    }
                }
                else if (line.find("mass = ") != std::string::npos) {
                    std::string massStr = line.substr(line.find("mass = ") + 7);
                    engine.mass = std::stod(massStr);
                }
                else if (line.find("maxThrust = ") != std::string::npos) {
                    std::string thrustStr = line.substr(line.find("maxThrust = ") + 12);
                    engine.Thrust = std::stod(thrustStr);
                }
                else if (line.find("PROPELLANT") != std::string::npos) {
                    while (std::getline(file, line) && line.find("}") == std::string::npos) {
                        if (line.find("name = ") != std::string::npos) {
                            std::string propName = trimString(line.substr(line.find("name = ") + 7));
                            if (propName == "LiquidFuel") { 
                                engineType = PartType::LFEngine; 
                            }
                            else if (propName == "Oxidizer") { 
                                engineType = PartType::LOXEngine; 
                            }
                            else if (propName == "MonoPropellant") { 
                                engineType = PartType::MPEngine; 
                            }
                            else if (propName == "XenonGas") { 
                                engineType = PartType::XenonEngine;
                            }
                        }
                    }
                }
            }
            partCatalogue.emplace_back(engineType,
                                       engine.name, 
                                       engine.mass, 
                                       0,
                                       0, 
                                       0, // crew
                                       0.0, 
                                       0.0, 
                                       0.0, 
                                       engine.Thrust, 
                                       0.0);
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(partsFolder / "Command")) {
        struct CmdPod {
            std::string name;
            double mass = 0.0;
            int crewCapacity = 0;
            double monoPropellantMass = 0.0;
        };
        CmdPod pod;
        if (entry.is_regular_file() && entry.path().extension() == ".cfg") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
                continue;
            }
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("name = ") != std::string::npos) {
                    if (pod.name.empty()) {
                        pod.name = trimString(line.substr(line.find("name = ") + 7));
                    }
                }
                else if (line.find("mass = ") != std::string::npos) {
                    std::string massStr = line.substr(line.find("mass = ") + 7);
                    pod.mass = std::stod(massStr);
                }
                else if (line.find("CrewCapacity = ") != std::string::npos) {
                    std::string crewStr = line.substr(line.find("CrewCapacity = ") + 15);
                    pod.crewCapacity = std::stoi(crewStr);
                }
                else if (line.find("RESOURCE") != std::string::npos) {
                    rtype = Resource::UNKNOWN;
                    while (std::getline(file, line) && line.find("}") == std::string::npos) {
                        if (line.find("name = ") != std::string::npos) {
                            std::string resName = trimString(line.substr(line.find("name = ") + 7));
                            if (resName == "MonoPropellant") { rtype = Resource::MP; }
                            else if (resName == "LiquidFuel") { rtype = Resource::LF; }
                            else if (resName == "Oxidizer") { rtype = Resource::OX; }
                        }
                        else if (line.find("maxAmount = ") != std::string::npos) {
                            std::string amountStr = line.substr(line.find("maxAmount = ") + 12);
                            switch (rtype) {
                                case Resource::MP: pod.monoPropellantMass = std::stod(amountStr); break;
                                default: break;
                            }
                        }
                    }
                }
            }
            partCatalogue.emplace_back(pod.crewCapacity > 0 ? PartType::CrewedCommandPod : PartType::DronePod,
                                       pod.name, 
                                       pod.mass, 
                                       0,
                                       0, 
                                       pod.crewCapacity,
                                       0.0, 
                                       0.0, 
                                       pod.monoPropellantMass, 
                                       0.0, 
                                       0.0);
        }
    }
}

#endif // LOADKSPPARTS_H
