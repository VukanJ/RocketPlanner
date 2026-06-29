#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <string_view>
#include "parts.h"

inline std::string partTypeToDisplayName(PartType type) {
    switch (type) {
        case PartType::CrewedCommandPod: return "Crewed Command Pod";
        case PartType::DronePod:         return "DronePod";
        case PartType::LFTank:           return "LF-Tank";
        case PartType::LFOXTank:         return "LOX-Tank";
        case PartType::MPTank:           return "MonoProp-Tank";
        case PartType::XenonTank:        return "Xenon-Tank";
        case PartType::JetEngine:        return "Jet Engine";
        case PartType::LFEngine:         return "LF-Engine";
        case PartType::LOXEngine:        return "LOX-Engine";
        case PartType::MPEngine:         return "MonoProp-Engine";
        case PartType::XenonEngine:      return "Xenon-Engine";
        case PartType::SolidBooster:     return "Solid Fuel Booster";
        default: return "Unknown Part Type";
    }
}

inline std::string_view partTypeToStr(PartType type) {
    switch (type) {
        case PartType::CrewedCommandPod: return "CrewedCommandPod";
        case PartType::DronePod:         return "DronePod";
        case PartType::LFTank:           return "LFTank";
        case PartType::LFOXTank:         return "LFOXTank";
        case PartType::MPTank:           return "MPTank";
        case PartType::XenonTank:        return "XenonTank";
        case PartType::JetEngine:        return "JetEngine";
        case PartType::LFEngine:         return "LFEngine";
        case PartType::LOXEngine:        return "LOXEngine";
        case PartType::MPEngine:         return "MPEngine";
        case PartType::XenonEngine:      return "XenonEngine";
        case PartType::SolidBooster:     return "SolidBooster";
        default:                         return "UNKNOWN";
    }
}

inline PartType partTypeFromStr(std::string_view str) {
    if (str == "CrewedCommandPod") return PartType::CrewedCommandPod;
    if (str == "DronePod")         return PartType::DronePod;
    if (str == "LFTank")           return PartType::LFTank;
    if (str == "LFOXTank")         return PartType::LFOXTank;
    if (str == "MPTank")           return PartType::MPTank;
    if (str == "XenonTank")        return PartType::XenonTank;
    if (str == "JetEngine")        return PartType::JetEngine;
    if (str == "LFEngine")         return PartType::LFEngine;
    if (str == "LOXEngine")        return PartType::LOXEngine;
    if (str == "MPEngine")         return PartType::MPEngine;
    if (str == "XenonEngine")      return PartType::XenonEngine;
    if (str == "SolidBooster")     return PartType::SolidBooster;
    return PartType::UNKNOWN;
}

#endif
