#ifndef HELPER_H
#define HELPER_H

#include "parts.h"

static bool isEngine(PartType type) {
    return type == PartType::LFEngine || type == PartType::LOXEngine || type == PartType::MPEngine || type == PartType::XenonEngine || type == PartType::SolidBooster;
}

#endif // HELPER_H
