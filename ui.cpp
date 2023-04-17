#include <cstddef>
#include <memory>
#include <optional>

#include <SDL_video.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <imgui.h>
#include <nfd.h>
#include <spdlog/spdlog.h>

#include "constants.hxx"
#include "sen.hxx"
#include "ui.hxx"
#include "util.hxx"

const char* SCALING_FACTORS[] = {"240p (1x)", "480p (2x)", "720p (3x)", "960p (4x)", "1200p (5x)"};

Ui::Ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        spdlog::error("Failed to init SDL2: {}", SDL_GetError());
        throw new std::runtime_error("Failed to init SDL2");
    }
    spdlog::info("Initialized SDL2");

#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window =
        SDL_CreateWindow("sen - NES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         NES_WIDTH * scale_factor, NES_HEIGHT * scale_factor, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync
    spdlog::info("Initialized SDL2 window and OpenGL context");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    spdlog::info("Initialized ImGui context");

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    spdlog::info("Using ImGui dark style");
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    this->rendering_context = std::make_unique<UiRenderingContext>(window, gl_context, io);
}

void Ui::Run() {
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui
        // wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
        // application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
        // application, or clear/overwrite your copy of the keyboard data. Generally you may always
        // pass all inputs to dear imgui, and hide them from your application based on those two
        // flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(rendering_context->window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ShowMenuBar();

        {
            if (show_cpu_registers && ImGui::Begin("CPU Registers", &show_cpu_registers)) {
            }
            ImGui::End();
        }

        {
            if (show_ppu_registers && ImGui::Begin("PPU Registers", &show_ppu_registers)) {
            }
            ImGui::End();
        }

        {
            if (show_vram && ImGui::Begin("VRAM", &show_vram)) {
            }
            ImGui::End();
        }

        {
            if (show_memory && ImGui::Begin("Memory", &show_memory)) {
            }
            ImGui::End();
        }

        {
            if (show_cart_info && ImGui::Begin("Cartridge Info", &show_cart_info)) {
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)rendering_context->io.DisplaySize.x,
                   (int)rendering_context->io.DisplaySize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(rendering_context->window);
    }
}
void Ui::ShowMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                nfdchar_t* selected_path = NULL;
                nfdresult_t result = NFD_OpenDialog("nes", NULL, &selected_path);

                if (result == NFD_OKAY) {
                    loaded_rom_file_path = std::make_optional<std::filesystem::path>(selected_path);
                    free(selected_path);
                    spdlog::info("Loading file {}", loaded_rom_file_path->c_str());

                    auto rom = read_binary_file(loaded_rom_file_path.value());
                    auto rom_args = RomArgs{rom};
                    emulator_context = std::make_optional<Sen>(rom_args);

                    auto title = fmt::format("Sen - {}", loaded_rom_file_path->filename().c_str());
                    SDL_SetWindowTitle(rendering_context->window, title.c_str());
                } else if (result == NFD_CANCEL) {
                    spdlog::debug("User pressed cancel");
                } else {
                    spdlog::error("Error: {}\n", NFD_GetError());
                }
            }
            if (ImGui::MenuItem("Open Recent")) {
            }
            if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                done = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation")) {
            if (ImGui::MenuItem("Start", nullptr, false,
                                !emulation_running && emulator_context.has_value())) {
                StartEmulation();
            }
            if (ImGui::MenuItem("Pause", nullptr, false, emulation_running)) {
                PauseEmulation();
            }
            if (ImGui::MenuItem("Reset", nullptr, false, emulation_running)) {
                ResetEmulation();
            }
            if (ImGui::MenuItem("Stop", nullptr, false, emulation_running)) {
                StopEmulation();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Scale")) {
                for (size_t i = 0; i < 5; i++) {
                    if (ImGui::MenuItem(SCALING_FACTORS[i], nullptr, scale_factor == i + 1)) {
                        scale_factor = i + 1;
                        spdlog::info("Changing window scale to {}", scale_factor);
                        SDL_SetWindowSize(rendering_context->window, NES_WIDTH * scale_factor,
                                          NES_HEIGHT * scale_factor);
                    }
                }

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("CPU Registers", nullptr, show_cpu_registers)) {
                show_cpu_registers = !show_cpu_registers;
            }
            if (ImGui::MenuItem("PPU Registers", nullptr, show_ppu_registers)) {
                show_ppu_registers = !show_ppu_registers;
            }
            if (ImGui::MenuItem("VRAM Viewer", nullptr, show_vram)) {
                show_vram = !show_vram;
            }
            if (ImGui::MenuItem("Memory Viewer", nullptr, show_memory)) {
                show_memory = !show_memory;
            }
            if (ImGui::MenuItem("Cartridge Info", nullptr, show_cart_info)) {
                show_cart_info = !show_cart_info;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Ui::StartEmulation() {}

void Ui::PauseEmulation() {}

void Ui::ResetEmulation() {}

void Ui::StopEmulation() {}
