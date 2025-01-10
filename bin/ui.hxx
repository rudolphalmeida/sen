#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <SDL2/SDL.h>

#include "constants.hxx"
#include "debugger.hxx"
#include "sen.hxx"
#include "filters.hxx"
#include "settings.hxx"

// static const std::tuple<int, ControllerKey> KEYMAP[0x8] = {
//     {GLFW_KEY_M, ControllerKey::A},          {GLFW_KEY_N, ControllerKey::B},
//     {GLFW_KEY_SPACE, ControllerKey::Select}, {GLFW_KEY_ENTER, ControllerKey::Start},
//     {GLFW_KEY_W, ControllerKey::Up},         {GLFW_KEY_S, ControllerKey::Down},
//     {GLFW_KEY_A, ControllerKey::Left},       {GLFW_KEY_D, ControllerKey::Right},
// };

class Ui {
   private:
    SenSettings settings{};

    SDL_Window* window{};
    SDL_GLContext gl_context{};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    unsigned int pattern_table_left_texture{};
    unsigned int pattern_table_right_texture{};
    unsigned int display_texture{};
    std::array<unsigned int, 64> sprite_textures{};

    std::unique_ptr<Filter> filter{};

    bool open{true};

    void LoadRomFile(const char* path);

    [[nodiscard]] static std::vector<Pixel> RenderPixelsForPatternTable(
        std::span<byte, 4096> pattern_table,
        std::span<byte, 32> nes_palette,
        int palette_id);

    void RenderUi();

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

    void HandleInput();

    void SetFilter(const FilterType filter);

    static void EmbraceTheDarkness();
    static void SetImGuiStyle();

   public:
    Ui();
    ~Ui();

    void MainLoop();

};
