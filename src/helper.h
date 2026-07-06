#ifndef HELPER_H
#define HELPER_H

#include <iostream>
#include "parts.h"

inline constexpr char RED[] = "\033[31m";
inline constexpr char BLUE[] = "\033[34m";
inline constexpr char NO_COLOR[] = "";

template<auto& Color = NO_COLOR, typename... Args>
void println(Args... args) {
    if constexpr (Color[0] != '\0') {
        std::cout << Color;
    }
    ((std::cout << args << ' '), ...);
    if constexpr (Color[0] != '\0') {
        std::cout << "\033[0m\n";
    }
    else {
        std::cout << '\n';
    }
}

static bool isEngine(PartType type) {
    return type == PartType::LFEngine || type == PartType::LOXEngine || type == PartType::MPEngine || type == PartType::XenonEngine || type == PartType::SolidBooster || type == PartType::JetEngine;
}

#endif // HELPER_H
