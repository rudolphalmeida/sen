#pragma once

#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_opengl.h>
#include <SDL_video.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "constants.hxx"
#include "controller.hxx"
#include "debugger.hxx"
#include "filters.hxx"
#include "sen.hxx"
#include "settings.hxx"

static const std::unordered_map<SDL_GameControllerButton, ControllerKey> KEYMAP = {
    // Assuming Nintendo style controllers. Only face buttons are used
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A, ControllerKey::A},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B, ControllerKey::B},
    // Allow using alternate buttons
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y, ControllerKey::A},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X, ControllerKey::B},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK, ControllerKey::Select},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START, ControllerKey::Start},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP, ControllerKey::Up},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN, ControllerKey::Down},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT, ControllerKey::Left},
    {SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT, ControllerKey::Right},
};

#define DEVICE_FORMAT AUDIO_F32
#define DEVICE_CHANNELS 1
#define DEVICE_SAMPLE_RATE 44100

#define MAX_AUDIO_FRAME_LAG 3

class AudioStreamQueue final: public AudioQueue {
  public:
    SDL_AudioStream* stream{};

    AudioStreamQueue() : stream(SDL_NewAudioStream(
            DEVICE_FORMAT,
            DEVICE_CHANNELS,
            NTSC_NES_CLOCK_FREQ,
            DEVICE_FORMAT,
            DEVICE_CHANNELS,
            DEVICE_SAMPLE_RATE
        )) {
        if (stream == nullptr) {
            spdlog::error("Failed to initialize SDL_AudioStream: {}", SDL_GetError());
            std::exit(-1);
        }
    }

    AudioStreamQueue(const AudioStreamQueue& other) = delete;
    AudioStreamQueue(AudioStreamQueue&& other) noexcept = default;
    AudioStreamQueue& operator=(const AudioStreamQueue& other) = delete;
    AudioStreamQueue& operator=(AudioStreamQueue&& other) noexcept = default;

    void push(const float sample) override {
        if (SDL_AudioStreamPut(stream, &sample, sizeof(float)) == -1) {
            spdlog::error("Failed to push sample into audio queue");
        }
    }

    void load_samples(uint8_t* output, size_t samples) override {
        if (SDL_AudioStreamGet(stream, output, samples * sizeof(float)) == -1) {
            spdlog::error("Failed to get {} samples from audio stream", samples);
        }
    }

    void clear() override {
        SDL_AudioStreamClear(stream);
    }

    ~AudioStreamQueue() override {
        SDL_FreeAudioStream(stream);
    }
};

class Ui {
  private:
    SenSettings settings;

    SDL_Window* window{};
    SDL_GLContext gl_context{};
    SDL_AudioStream* audio_stream{};
    SDL_GameController* controller{};

    std::optional<std::filesystem::path> loaded_rom_file_path = std::nullopt;
    std::shared_ptr<Sen> emulator_context{};
    bool emulation_running{false};

    Debugger debugger{};

    GLuint pattern_table_left_texture{};
    GLuint pattern_table_right_texture{};
    GLuint display_texture{};
    std::array<GLuint, 64> sprite_textures{};

    std::unique_ptr<Filter> filter{};
    std::shared_ptr<AudioQueue> audio_queue{};

    int audio_frame_delay{MAX_AUDIO_FRAME_LAG};
    bool open{true};

    byte pressed_nes_keys{};

    void init_sdl();
    void init_sdl_audio();
    void init_imgui() const;

    void load_rom_file(const char* path);
    static SDL_GameController* find_controllers();

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
