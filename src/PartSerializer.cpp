#include "PartSerializer.h"
#include "utils.h"
#include "helper.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void to_json(json& j, const ResourceContainer& rc) {
    j = json{
        {"liquidFuel",     rc.liquidFuel},
        {"oxidizer",       rc.oxidizer},
        {"monoPropellant", rc.monoPropellant},
        {"solidFuel",      rc.solidFuel},
        {"xenonGas",       rc.xenonGas}
    };
}

void from_json(const json& j, ResourceContainer& rc) {
    rc.liquidFuel     = j.value("liquidFuel", 0.0);
    rc.oxidizer       = j.value("oxidizer", 0.0);
    rc.monoPropellant = j.value("monoPropellant", 0.0);
    rc.solidFuel      = j.value("solidFuel", 0.0);
    rc.xenonGas       = j.value("xenonGas", 0.0);
}

void to_json(json& j, const EngineISPInfo& ei) {
    json curve = json::array();
    for (const auto& [p, isp] : ei.isp) {
        curve.push_back({p, isp});
    }
    j = json{
        {"vacuumISP",              ei.vacuumISP},
        {"fuelConsumptionRate_UPS", ei.fuelConsumptionRate_UPS},
        {"ispCurve",               curve}
    };
}

void from_json(const json& j, EngineISPInfo& ei) {
    ei.vacuumISP = j.value("vacuumISP", 0.0);
    ei.fuelConsumptionRate_UPS = j.value("fuelConsumptionRate_UPS", 0.0);
    ei.isp.clear();
    for (const auto& pair : j.at("ispCurve")) {
        ei.isp.emplace_back(pair[0].get<double>(), pair[1].get<double>());
    }
}

void to_json(json& j, const PartProperty& p) {
    j = json::object();
    j["type"]      = std::string(partTypeToStr(p.type));
    j["title"]     = p.title;
    j["mass"]      = p.mass;
    j["attTop"]    = p.attTop;
    j["attBottom"] = p.attBottom;
    j["length"]    = p.length;
    if (p.MaxCrew > 0) {
        j["maxCrew"] = p.MaxCrew;
    }
    if (p.MaxThrustkN > 0.0) {
        j["maxThrustkN"] = p.MaxThrustkN;
    }
    if (isEngine(p.type)) {
        j["enginePerf"] = p.enginePerf;
    }
    if (p.hasResources) {
        j["resources"] = p.resources;
    }
}

void savePartsToJSON(const std::filesystem::path& jsonPath, const std::vector<PartProperty>& parts) {
    json arr = json::array();
    for (const auto& p : parts) {
        arr.push_back(p);
    }
    std::ofstream out(jsonPath);
    if (!out.is_open()) {
        std::cerr << "ERROR: Could not write " << jsonPath << '\n';
        return;
    }
    out << arr.dump(2) << '\n';
    std::cout << "Saved " << parts.size() << " parts to " << jsonPath << '\n';
}

bool loadPartsFromJSON(const std::filesystem::path& jsonPath, std::vector<PartProperty>& parts) {
    std::ifstream in(jsonPath);
    if (!in.is_open()) {
        std::cerr << "ERROR: Could not open " << jsonPath << '\n';
        return false;
    }

    json arr;
    try {
        in >> arr;
    } catch (const json::exception& e) {
        std::cerr << "ERROR: Failed to parse " << jsonPath << ": " << e.what() << '\n';
        return false;
    }

    if (!arr.is_array()) {
        std::cerr << "ERROR: Expected JSON array in " << jsonPath << '\n';
        return false;
    }

    for (const auto& item : arr) {
        auto type        = partTypeFromStr(item.value("type", "UNKNOWN"));
        auto title       = item.value("title", std::string());
        auto mass        = item.value("mass", 0.0);
        int attTop       = item.value("attTop", 0);
        int attBottom    = item.value("attBottom", 0);
        float length     = item.value("length", 0.0f);
        int maxCrew      = item.value("maxCrew", 0);
        double maxThrust = item.value("maxThrustkN", 0.0);

        ResourceContainer resources;
        if (item.contains("resources")) {
            from_json(item["resources"], resources);
        }

        EngineISPInfo enginePerf;
        if (item.contains("enginePerf")) {
            from_json(item["enginePerf"], enginePerf);
        }

        parts.emplace_back(type, title, mass, attTop, attBottom,
                           length, maxCrew, resources, maxThrust, enginePerf);
    }

    std::cout << "Loaded " << parts.size() << " parts from " << jsonPath << '\n';
    return true;
}
