#include <iostream>
#include <filesystem>

#include "WindowSimulator.h"
#include "kspConstants.h"
#include "rocket.h"
#include "cmdargs.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

std::string findKSP() {
    const char* home = std::getenv("HOME");
    std::filesystem::path homePath(home);

    if (auto kspSteamPath = homePath / ".local/share/Steam/steamapps/common/Kerbal Space Program"; std::filesystem::exists(kspSteamPath)) {
        std::cout << "KSP found in Steam directory." << std::endl;
        std::cout << "Path: " << kspSteamPath << std::endl;
        return kspSteamPath.string();
    }

    throw std::runtime_error("KSP installation not found. Please ensure KSP is installed and try again.");
    return "";
}

static void run_interactive() {
    if (!glfwInit()) {
        return;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Rocket Optimizer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
#ifdef AGAVE_FONT_PATH
    if (std::filesystem::exists(AGAVE_FONT_PATH)) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(AGAVE_FONT_PATH, 16.0f, nullptr, io.Fonts->GetGlyphRangesGreek());
        if (font) io.FontDefault = font;
    }
#endif
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    const auto kspPath = findKSP();
    Rocket rocket(kspPath, "Rocket Optimizer");
    WindowSimulator ws(rocket.allEngines());

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ws.render();
        ws.renderKinematics();
        ws.renderBodySelector();
        ws.renderPictogram();


        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char** argv) {
    CmdArgs args;
    args.add_flag("-i", "Open interactive GUI window");
    args.add_flag("--interactive", "Open interactive GUI window");

    if (!args(argc, argv)) {
        std::cerr << "ERROR: Invalid command line option(s)\n";
        args.print_help();
        return EXIT_FAILURE;
    }
    if (args.is_flag_set("-h") || args.is_flag_set("--help")) {
        return EXIT_SUCCESS;
    }

    if (args.is_flag_set("-i") || args.is_flag_set("--interactive")) {
        run_interactive();
        return EXIT_SUCCESS;
    }

    const auto kspPath = findKSP();
    Rocket rocket(kspPath, "Test Rocket");

    rocket.setRootPart("Mk2 Lander Can");
    rocket.construct(10000, 1.7, 1.3, KspSystem::Eve.surfaceGravity, KspSystem::Eve.seaLevel_atm);
    return EXIT_SUCCESS;
}
