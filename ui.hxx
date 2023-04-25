#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "constants.hxx"
#include "debugger.hxx"
#include "sen.hxx"
#include "util.hxx"

struct Pixel {
    byte r{};
    byte g{};
    byte b{};
};

class Ui {
   private:
    GLFWwindow* window{};
    unsigned int scale_factor{3};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    bool show_cpu_registers{false};
    bool show_ppu_registers{false};
    bool show_pattern_tables{false};
    bool show_cpu_memory{false};
    bool show_ppu_memory{false};
    bool show_cart_info{false};

    unsigned int pattern_table_left_texture{};
    unsigned int pattern_table_right_texture{};
    std::vector<Pixel> RenderPixelsForPatternTable(std::span<byte, 4096> pattern_table) const;

    void ShowMenuBar();

    void StartEmulation();
    void PauseEmulation();
    void ResetEmulation();
    void StopEmulation();

   public:
    Ui();

    void Run();

    ~Ui() {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
