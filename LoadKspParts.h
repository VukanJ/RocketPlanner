#ifndef LOADKSPPARTS_H
#define LOADKSPPARTS_H

#include <filesystem>
#include <optional>

#include "parts.h"

struct KSPPart {
    PartType type = PartType::UNKNOWN;
    std::string name;
    double mass = 0.0;
    int attTop = 0; // Attachment size class for top node
    int attBottom = 0;
    float attTopY = 0.0f; // Top node position
    float attBottomY = 0.0f; // Bottom node position
    ResourceContainer resources;
};
struct Engine : public KSPPart {
    double Thrust = 0.0;
};
struct CmdPod : public KSPPart {
    int crewCapacity = 0;
};
enum ResourceType { LF, OX, MP, SOLID, XE, UNKNOWN };


std::string trimString(const std::string& str);

int parseNodeSize(const std::string& line);

void loadPartCatalogueFromKSP(const std::filesystem::path& ksp_path, std::vector<Part>& partCatalogue);

ResourceType parseResourceType(const std::string& resourceName);

std::optional<KSPPart> loadFuelTankPart(const std::filesystem::path&);
std::optional<Engine> loadEnginePart(const std::filesystem::path&);
std::optional<CmdPod> loadCommandPodPart(const std::filesystem::path&);

#endif // LOADKSPPARTS_H
