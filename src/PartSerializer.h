#ifndef PARTSERIALIZER_H
#define PARTSERIALIZER_H

#include <filesystem>
#include <vector>
#include "parts.h"

bool loadPartsFromJSON(const std::filesystem::path& jsonPath, std::vector<PartProperty>& parts);
void savePartsToJSON(const std::filesystem::path& jsonPath, const std::vector<PartProperty>& parts);

#endif
