#pragma once

#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <miniaudio.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>
#include <libconfig.h++>

#include "constants.hxx"
#include "debugger.hxx"
#include "sen.hxx"
#include "util.hxx"

#define CRT_SYSTEM CRT_SYSTEM_NES
#include "crt_core.h"

constexpr int DEFAULT_SCALE_FACTOR = 4;

enum class UiStyle {
    Classic = 0,
    Light = 1,
    Dark = 2,
    SuperDark = 3,
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

enum class FilterType {
    NoFilter = 0,
    Ntsc = 1,
};

struct SenSettings {
    libconfig::Config cfg{};

    [[nodiscard]] int Width() const { return cfg.getRoot()["ui"]["width"]; }
    void Width(const int width) const { cfg.getRoot()["ui"]["width"] = width; }

    [[nodiscard]] int Height() const { return cfg.getRoot()["ui"]["height"]; }
    void Height(const int height) const { cfg.getRoot()["ui"]["height"] = height; }

    [[nodiscard]] int ScaleFactor() const { return cfg.getRoot()["ui"]["scale"]; }
    void SetScale(const int scale) const { cfg.getRoot()["ui"]["scale"] = scale; }

    [[nodiscard]] FilterType GetFilterType() const { return static_cast<enum FilterType>(static_cast<int>(cfg.getRoot()["ui"]["filter"])); }
    void SetFilterType(enum FilterType filter) const { cfg.getRoot()["ui"]["filter"] = static_cast<int>(filter); }

    [[nodiscard]] UiStyle GetUiStyle() const {
        return static_cast<enum UiStyle>(static_cast<int>(cfg.getRoot()["ui"]["style"]));
    }

    void SetUiStyle(enum UiStyle style) const {
        cfg.getRoot()["ui"]["style"] = static_cast<int>(style);
    }

    [[nodiscard]] int GetOpenPanels() const { return cfg.getRoot()["ui"]["open_panels"]; }

    void SetOpenPanels(const int open_panels) const {
        cfg.getRoot()["ui"]["open_panels"] = open_panels;
    }

    void TogglePanel(UiPanel panel) const {
        cfg.getRoot()["ui"]["open_panels"] =
            static_cast<int>(cfg.getRoot()["ui"]["open_panels"]) ^ (1 << static_cast<int>(panel));
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

struct PostProcessedData {
    Pixel * data;
    int width{};
    int height{};
};

class Filter {
   public:
    virtual ~Filter() = default;
    virtual PostProcessedData PostProcess(std::span<unsigned short, 61440> nes_pixels,
                                          int scale_factor) = 0;
};

class NoFilter final : public Filter {
   public:
    NoFilter() : pixels(NES_WIDTH * NES_HEIGHT) {}

    PostProcessedData PostProcess(std::span<unsigned short, 61440> nes_pixels,
                                  int scale_factor) override;

   private:
    std::vector<Pixel> pixels{};
};

class NtscFilter final : public Filter {
   public:
    explicit NtscFilter(const int initial_scale_factor)
        : pixels(NES_WIDTH * NES_HEIGHT * initial_scale_factor * initial_scale_factor),
          scale_factor{initial_scale_factor} {
        crt_init(&crt, NES_WIDTH * initial_scale_factor, NES_HEIGHT * initial_scale_factor,
                 CRT_PIX_FORMAT_RGB, reinterpret_cast<unsigned char*>(pixels.data()));
        crt.blend = 1;
        crt.scanlines = 1;
    }

    PostProcessedData PostProcess(std::span<unsigned short, 61440> nes_pixels,
                                  int scale_factor) override;

   private:
    std::vector<Pixel> pixels{};
    int scale_factor{};

    CRT crt{};
    NTSC_SETTINGS ntsc{};
    int noise = 2;
    int hue = 350;
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
    ma_device device{};

    GLFWwindow* window{};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    unsigned int pattern_table_left_texture{};
    unsigned int pattern_table_right_texture{};
    unsigned int display_texture{};
    std::array<unsigned int, 64> sprite_textures{};

    std::unique_ptr<Filter> filter{};

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
    void ShowDebugger();

    void DrawSprite(size_t index,
                    const SpriteData& sprite,
                    const std::span<byte, 0x20>& palettes) const;
    void ShowOam();

    void StartEmulation();
    void PauseEmulation();
    void ResetEmulation();
    void StopEmulation();

    void HandleInput() const;

    void SetFilter(const FilterType filter) {
        switch (filter) {
            case FilterType::NoFilter:
                this->filter = std::make_unique<NoFilter>();
                break;
            case FilterType::Ntsc:
                this->filter = std::make_unique<NtscFilter>(settings.ScaleFactor());
                break;
        }
    }

    static void EmbraceTheDarkness() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
    }

    static void SetImGuiStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.00f, 8.00f);
        style.FramePadding = ImVec2(5.00f, 4.00f);
        style.CellPadding = ImVec2(6.00f, 6.00f);
        style.ItemSpacing = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
        style.IndentSpacing = 25;
        style.ScrollbarSize = 15;
        style.GrabMinSize = 10;
        style.WindowBorderSize = 1;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 1;
        style.FrameBorderSize = 1;
        style.TabBorderSize = 1;
        style.WindowRounding = 7;
        style.ChildRounding = 4;
        style.FrameRounding = 3;
        style.PopupRounding = 4;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 3;
        style.LogSliderDeadzone = 4;
        style.TabRounding = 4;
    }

   public:
    Ui();

    void RunForSamples(uint32_t cycles);
    void MainLoop();

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
        ma_device_uninit(&device);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void RenderUi();
};
