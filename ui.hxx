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

static const Pixel PALETTE_COLORS[0x40] = {
    Pixel{84, 84, 84},    Pixel{0, 30, 116},    Pixel{8, 16, 144},    Pixel{48, 0, 136},
    Pixel{68, 0, 100},    Pixel{92, 0, 48},     Pixel{84, 4, 0},      Pixel{60, 24, 0},
    Pixel{32, 42, 0},     Pixel{8, 58, 0},      Pixel{0, 64, 0},      Pixel{0, 60, 0},
    Pixel{0, 50, 60},     Pixel{0, 0, 0},       Pixel{152, 150, 152}, Pixel{8, 76, 196},
    Pixel{48, 50, 236},   Pixel{92, 30, 228},   Pixel{136, 20, 176},  Pixel{160, 20, 100},
    Pixel{152, 34, 32},   Pixel{120, 60, 0},    Pixel{84, 90, 0},     Pixel{40, 114, 0},
    Pixel{8, 124, 0},     Pixel{0, 118, 40},    Pixel{0, 102, 120},   Pixel{0, 0, 0},
    Pixel{236, 238, 236}, Pixel{76, 154, 236},  Pixel{120, 124, 236}, Pixel{176, 98, 236},
    Pixel{228, 84, 236},  Pixel{236, 88, 180},  Pixel{236, 106, 100}, Pixel{212, 136, 32},
    Pixel{160, 170, 0},   Pixel{116, 196, 0},   Pixel{76, 208, 32},   Pixel{56, 204, 108},
    Pixel{56, 180, 204},  Pixel{60, 60, 60},    Pixel{236, 238, 236}, Pixel{168, 204, 236},
    Pixel{188, 188, 236}, Pixel{212, 178, 236}, Pixel{236, 174, 236}, Pixel{236, 174, 212},
    Pixel{236, 180, 176}, Pixel{228, 196, 144}, Pixel{204, 210, 120}, Pixel{180, 222, 120},
    Pixel{168, 226, 144}, Pixel{152, 226, 180}, Pixel{160, 214, 228}, Pixel{160, 162, 160}};

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

    std::vector<Pixel> RenderPixelsForPatternTable(std::span<byte, 4096> pattern_table,
                                                   std::span<byte, 32> nes_palette,
                                                   int palette_id) const;

    void ShowMenuBar();
    bool show_imgui_demo{false};

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
