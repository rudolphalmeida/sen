#pragma once

#include <cstring>
#include <filesystem>
#include <iterator>
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

#include <bitset>
#include <libconfig.h++>

#include "constants.hxx"
#include "debugger.hxx"
#include "sen.hxx"
#include "util.hxx"

constexpr int DEFAULT_SCALE_FACTOR = 4;

enum class UiStyle {
    Classic = 0,
    Light = 1,
    Dark = 2,
};

constexpr int NUM_PANELS = 8;
enum class UiPanel {
    Registers = 0,
    PatternTables = 1,
    PpuMemory = 2,
    CpuMemory = 3,
    Sprites = 4,
    Opcodes = 5,
    Debugger = 6,
    Logs = 7,
};

struct SenSettings {
    libconfig::Config cfg{};

    [[nodiscard]] int Width() const { return cfg.getRoot()["ui"]["width"]; }
    void Width(int width) const { cfg.getRoot()["ui"]["width"] = width; }

    [[nodiscard]] int Height() const { return cfg.getRoot()["ui"]["height"]; }
    void Height(int height) const { cfg.getRoot()["ui"]["height"] = height; }

    [[nodiscard]] int ScaleFactor() const { return cfg.getRoot()["ui"]["scale"]; }
    void SetScale(int scale) const { cfg.getRoot()["ui"]["scale"] = scale; }

    [[nodiscard]] UiStyle GetUiStyle() const {
        return static_cast<enum UiStyle>((int)cfg.getRoot()["ui"]["style"]);
    }

    void SetUiStyle(enum UiStyle style) const {
        cfg.getRoot()["ui"]["style"] = static_cast<int>(style);
    }

    [[nodiscard]] int GetOpenPanels() const {
        return cfg.getRoot()["ui"]["open_panels"];
    }

    void SetOpenPanels(const int open_panels) const {
        cfg.getRoot()["ui"]["open_panels"] = open_panels;
    }

    void TogglePanel(UiPanel panel) const {
        cfg.getRoot()["ui"]["open_panels"] = static_cast<int>(cfg.getRoot()["ui"]["open_panels"]) ^ (1 << static_cast<int>(panel));
    }

    void RecentRoms(std::vector<const char*>& paths) const {
        const libconfig::Setting& recents = cfg.getRoot()["ui"]["recents"];
        paths.resize(recents.getLength());
        std::copy(recents.begin(), recents.end(), paths.begin());
    }

    void PushRecentPath(const char* path) const {
        libconfig::Setting& recents = cfg.getRoot()["ui"]["recents"];
        for (int i = 0; i < recents.getLength(); i++) {
            if (strcmp(recents[i], path) == 0) {
                return;
            }
        }
        recents.add(libconfig::Setting::TypeString) = path;
    }
};

struct Pixel {
    byte r{};
    byte g{};
    byte b{};
};

static constexpr Pixel PALETTE_COLORS[0x40] = {
    Pixel{84, 84, 84},    Pixel{0, 30, 116},    Pixel{8, 16, 144},    Pixel{48, 0, 136},
    Pixel{68, 0, 100},    Pixel{92, 0, 48},     Pixel{84, 4, 0},      Pixel{60, 24, 0},
    Pixel{32, 42, 0},     Pixel{8, 58, 0},      Pixel{0, 64, 0},      Pixel{0, 60, 0},
    Pixel{0, 50, 60},     Pixel{0, 0, 0},       Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{152, 150, 152}, Pixel{8, 76, 196},    Pixel{48, 50, 236},   Pixel{92, 30, 228},
    Pixel{136, 20, 176},  Pixel{160, 20, 100},  Pixel{152, 34, 32},   Pixel{120, 60, 0},
    Pixel{84, 90, 0},     Pixel{40, 114, 0},    Pixel{8, 124, 0},     Pixel{0, 118, 40},
    Pixel{0, 102, 120},   Pixel{0, 0, 0},       Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{236, 238, 236}, Pixel{76, 154, 236},  Pixel{120, 124, 236}, Pixel{176, 98, 236},
    Pixel{228, 84, 236},  Pixel{236, 88, 180},  Pixel{236, 106, 100}, Pixel{212, 136, 32},
    Pixel{160, 170, 0},   Pixel{116, 196, 0},   Pixel{76, 208, 32},   Pixel{56, 204, 108},
    Pixel{56, 180, 204},  Pixel{60, 60, 60},    Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{236, 238, 236}, Pixel{168, 204, 236}, Pixel{188, 188, 236}, Pixel{212, 178, 236},
    Pixel{236, 174, 236}, Pixel{236, 174, 212}, Pixel{236, 180, 176}, Pixel{228, 196, 144},
    Pixel{204, 210, 120}, Pixel{180, 222, 120}, Pixel{168, 226, 144}, Pixel{152, 226, 180},
    Pixel{160, 214, 228}, Pixel{160, 162, 160}, Pixel{0, 0, 0},       Pixel{0, 0, 0},
};

static const std::tuple<int, ControllerKey> KEYMAP[0x8] = {
    {GLFW_KEY_M, ControllerKey::A},          {GLFW_KEY_N, ControllerKey::B},
    {GLFW_KEY_SPACE, ControllerKey::Select}, {GLFW_KEY_ENTER, ControllerKey::Start},
    {GLFW_KEY_W, ControllerKey::Up},         {GLFW_KEY_S, ControllerKey::Down},
    {GLFW_KEY_A, ControllerKey::Left},       {GLFW_KEY_D, ControllerKey::Right},
};

class Ui {
   private:
    SenSettings settings{};

    GLFWwindow* window{};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    unsigned int pattern_table_left_texture{};
    unsigned int pattern_table_right_texture{};
    unsigned int display_texture{};
    std::array<unsigned int, 64> sprite_textures{};

    void LoadRomFile(const char* path) {
        loaded_rom_file_path = std::make_optional<std::filesystem::path>(path);
        spdlog::info("Loading file {}", loaded_rom_file_path->string());

        auto rom = ReadBinaryFile(loaded_rom_file_path.value());
        auto rom_args = RomArgs{rom};
        emulator_context = std::make_shared<Sen>(rom_args);
        debugger = Debugger(emulator_context);

        auto title = fmt::format("Sen - {}", loaded_rom_file_path->filename().string());
        glfwSetWindowTitle(window, title.c_str());
    }

    [[nodiscard]] static std::vector<Pixel> RenderPixelsForPatternTable(
        std::span<byte, 4096> pattern_table,
        std::span<byte, 32> nes_palette,
        int palette_id);

    std::array<bool, NUM_PANELS> open_panels{};

    void ShowMenuBar();
    void ShowRegisters();
    void ShowPatternTables();
    void ShowPpuMemory();
    void ShowOpcodes();

    void DrawSprite(size_t index, const SpriteData& sprite, const std::span<byte, 0x20>& palettes) const;
    void ShowOam();

    void StartEmulation();
    void PauseEmulation();
    void ResetEmulation();
    void StopEmulation();

    void HandleInput() const;

   public:
    Ui();

    void Run();

    ~Ui() {
        int panels = 0;
        for (int i = 0; i < NUM_PANELS; i++) {
            if (open_panels[i]) {
                panels |= (1 << i);
            }
        }
        settings.SetOpenPanels(panels);

        try {
            settings.cfg.writeFile("test.cfg");
        } catch (const libconfig::FileIOException& e) {
            spdlog::error("Failed to save settings: {}", e.what());
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void RenderUi();
};
