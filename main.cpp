#include <iostream>
#include <filesystem>

#include "kspConstants.h"
#include "rocket.h"

std::string findKSP() {
    const char* home = std::getenv("HOME");
    std::filesystem::path homePath(home);

    // Check for steam 
    if (auto kspSteamPath = homePath / ".local/share/Steam/steamapps/common/Kerbal Space Program"; std::filesystem::exists(kspSteamPath)) {
        std::cout << "KSP found in Steam directory." << std::endl;
        std::cout << "Path: " << kspSteamPath << std::endl;
        return kspSteamPath.string();
    }

    throw std::runtime_error("KSP installation not found. Please ensure KSP is installed and try again.");
    return "";
}

int main() {

    const auto kspPath = findKSP();
    Rocket rocket(kspPath, "Test Rocket");

    rocket.setRootPart("Mk2 Lander Can");
    rocket.construct(5000, 2, 2.0, Constants::g0_eve);
    //rocket.printConfig();
}
