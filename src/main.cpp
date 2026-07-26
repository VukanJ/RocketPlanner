#include <iostream>
#include <filesystem>
#include <fstream>

#include "Calendar.h"
#include "WindowSimulator.h"
#include "gui.h"
#include "kspConstants.h"
#include "rocket.h"
#include "cmdargs.h"

#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>

std::string findKSP() {
    const char* home = std::getenv("HOME");
    std::filesystem::path homePath(home);

    if (auto kspSteamPath = homePath / ".local/share/Steam/steamapps/common/Kerbal Space Program"; std::filesystem::exists(kspSteamPath)) {
        std::cout << "KSP found in Steam directory." << std::endl;
        std::cout << "Path: " << kspSteamPath << std::endl;
        return kspSteamPath.string();
    }

    std::cout << "KSP installation not found. Using cached parts if available." << std::endl;
    return "";
}

static std::filesystem::path getConfigPath() {
    const char* home = std::getenv("HOME");
    auto dir = std::filesystem::path(home) / ".rocket_planner";
    std::filesystem::create_directories(dir);
    return dir / "config.json";
}

struct WindowSize { int width; int height; };

static WindowSize loadWindowSize() {
    constexpr WindowSize defaults{1280, 720};
    auto path = getConfigPath();
    if (!std::filesystem::exists(path)) { return defaults; }

    std::ifstream file(path);
    if (!file.is_open()) { return defaults; }

    try {
        auto json = nlohmann::json::parse(file);
        return {
            json.value("window_width", defaults.width),
            json.value("window_height", defaults.height)
        };
    } catch (...) {
        return defaults;
    }
}

static void saveWindowSize(GLFWwindow* window) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);

    nlohmann::json json;
    json["window_width"] = w;
    json["window_height"] = h;

    std::ofstream file(getConfigPath());
    if (file.is_open()) {
        file << json.dump(4) << std::endl;
    }
}

static void run_interactive() {
    if (!glfwInit()) {
        return;
    }

    GLFWwindow* window = [&]() {
        auto [w, h] = loadWindowSize();
        return glfwCreateWindow(w, h, "Rocket Optimizer", nullptr, nullptr);
    }();
    if (!window) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window, 400, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
#ifdef AGAVE_FONT_PATH
    if (std::filesystem::exists(AGAVE_FONT_PATH)) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(AGAVE_FONT_PATH, 19.0f, nullptr, io.Fonts->GetGlyphRangesGreek());
        if (font) io.FontDefault = font;
    }
#endif
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    const auto kspPath = findKSP();
    Rocket rocket(kspPath, "Rocket Optimizer");
    //WindowSimulator ws(rocket.allEngines());
    GUI gui(rocket.allEngines());

    glfwSetWindowUserPointer(window, &gui);
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        gui.onWindowResized(w, h);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        gui.render();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    saveWindowSize(window);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char** argv) {
    CmdArgs args;
    args.add_flag("-i", "Open interactive GUI window");
    args.add_flag("-x", "Debug mode");
    args.add_flag("--interactive", "Open interactive GUI window");

    if (!args(argc, argv)) {
        std::cerr << "ERROR: Invalid command line option(s)\n";
        args.print_help();
        return EXIT_FAILURE;
    }
    if (args.is_flag_set("-h") || args.is_flag_set("--help")) {
        return EXIT_SUCCESS;
    }

    if (args.is_flag_set("-x")) {
        auto init = get_local_position(KspSystem::Kerbin.orbit);
        std::cout << "Initial position: " << init.first.transpose() << std::endl;
        std::cout << "Initial velocity: " << init.second.transpose() << std::endl;
        return EXIT_SUCCESS;
    }

    if (true || args.is_flag_set("-i") || args.is_flag_set("--interactive")) {
        run_interactive();
        return EXIT_SUCCESS;
    }

    const auto kspPath = findKSP();
    Rocket rocket(kspPath, "Test Rocket");

    rocket.setRootPart("Mk2 Lander Can");
    rocket.construct(10000, 1.7, 1.3, KspSystem::Eve.surfaceGravity, KspSystem::Eve.seaLevel_atm);
    return EXIT_SUCCESS;
}
