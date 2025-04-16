#pragma once

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "apu.hxx"
#include "constants.hxx"
#include "controller.hxx"
#include "debugger.hxx"
#include "filters.hxx"
#include "sen.hxx"
#include "settings.hxx"
#include "spdlog_imgui_sink.h"

static const std::unordered_map<SDL_GamepadButton, ControllerKey> KEYMAP = {
    // Assuming Nintendo style controllers. Only face buttons are used
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_EAST, ControllerKey::A},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH, ControllerKey::B},
    // Allow using alternate buttons
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_WEST, ControllerKey::A},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_NORTH, ControllerKey::B},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_BACK, ControllerKey::Select},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_START, ControllerKey::Start},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_UP, ControllerKey::Up},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_DOWN, ControllerKey::Down},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_LEFT, ControllerKey::Left},
    {SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_RIGHT, ControllerKey::Right},
};

constexpr int DEVICE_CHANNELS = 1;
constexpr int DEVICE_SAMPLE_RATE = 44100;
constexpr int MAX_AUDIO_FRAME_LAG = 3;

constexpr SDL_AudioSpec NES_AUDIO_SPEC = {
    .format = SDL_AUDIO_F32,
    .channels = DEVICE_CHANNELS,
    .freq = NTSC_NES_CLOCK_FREQ,
};
constexpr SDL_AudioSpec OUTPUT_DEVICE_SPEC = {
    .format = SDL_AUDIO_F32,
    .channels = DEVICE_CHANNELS,
    .freq = DEVICE_SAMPLE_RATE,
};

class AudioStreamQueue final: public AudioQueue {
  public:
    SDL_AudioStream* stream{};
    SDL_AudioDeviceID device_id;

    explicit AudioStreamQueue(const SDL_AudioDeviceID device_id): stream(SDL_CreateAudioStream(&NES_AUDIO_SPEC, &OUTPUT_DEVICE_SPEC)), device_id{device_id} {
        if (stream == nullptr) {
            spdlog::error("Failed to initialize audio stream: {}", SDL_GetError());
            std::exit(-1);
        }

        SDL_BindAudioStream(device_id, stream);
    }

    AudioStreamQueue(const AudioStreamQueue& other) = delete;
    AudioStreamQueue(AudioStreamQueue&& other) noexcept = default;
    AudioStreamQueue& operator=(const AudioStreamQueue& other) = delete;
    AudioStreamQueue& operator=(AudioStreamQueue&& other) noexcept = default;

    void push(const float sample) override {
        if (!SDL_PutAudioStreamData(stream, &sample, sizeof(float))) {
            spdlog::error("Failed to push sample into audio queue");
        }
    }

    void resume() const {
        SDL_ResumeAudioDevice(device_id);
    }

    void pause() const {
        SDL_PauseAudioDevice(device_id);
    }

    void clear() const {
        pause();
        SDL_ClearAudioStream(stream);
    }

    void destroy() {
        SDL_PauseAudioDevice(device_id);
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    ~AudioStreamQueue() override {
        if (stream) {
            destroy();
        }
    }
};

class Ui {
    std::shared_ptr<imgui_sink<>> sink;

    SenSettings settings;

    SDL_Window* window{};
    SDL_GLContext gl_context{};
    SDL_Gamepad* controller{};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    GLuint pattern_table_left_texture{};
    GLuint pattern_table_right_texture{};
    GLuint game_texture{};
    std::array<GLuint, 64> sprite_textures{};

    std::unique_ptr<Filter> filter;
    std::shared_ptr<AudioStreamQueue> audio_queue;

    int audio_frame_delay{MAX_AUDIO_FRAME_LAG};
    bool open{true};

    byte pressed_nes_keys{};

    void init_sdl();
    void init_sdl_audio();
    void init_imgui() const;

    void load_rom_file(const char* path);
    static SDL_Gamepad* find_controllers();

    [[nodiscard]] static std::vector<Pixel> render_pattern_table(
        const std::array<byte, 4096>& pattern_table,
        const std::array<byte, 32>& nes_palette,
        int palette_id
    );

    void render_ui();

    void show_menu_bar();
    void show_registers();
    void show_pattern_tables();
    void show_ppu_memory();
    void show_opcodes();
    void show_debugger();
    void show_volume_control();
    void show_logs();

    void
    draw_sprite(size_t index, const SpriteData& sprite, const std::array<byte, 32>& palettes) const;
    void show_oam();

    void start_emulation();
    void pause_emulation();
    void reset_emulation();
    void stop_emulation();

    void handle_sdl_events();

    void set_filter(FilterType filter);

    static void EmbraceTheDarkness();
    static void set_imgui_style();

  public:
    Ui();
    ~Ui();

    void run();
};
