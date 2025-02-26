#include "ui.hxx"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <nfd.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_audio.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>

#include "IconsFontAwesome6.h"
#include "controller.hxx"
#include "imgui_memory_editor.h"
#include "ImGuiNotify.hpp"
#include "settings.hxx"
#include "util.hxx"

static constexpr const char* const SCALING_FACTORS[] = {"240p (1x)", "480p (2x)", "720p (3x)", "960p (4x)", "1200p (5x)"};
constexpr auto GLSL_VERSION = "#version 130";

static void InitTexture(const GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

Ui::Ui() {
    init_sdl();
    init_imgui();
    init_sdl_audio();

    // Setup OpenGL textures for drawing
    glGenTextures(1, &pattern_table_left_texture);
    InitTexture(pattern_table_left_texture);

    glGenTextures(1, &pattern_table_right_texture);
    InitTexture(pattern_table_right_texture);

    glGenTextures(1, &display_texture);
    InitTexture(display_texture);

    glGenTextures(64, sprite_textures.data());
    for (int i = 0; i < 64; i++) {
        InitTexture(sprite_textures[i]);
    }

    set_filter(settings.GetFilterType());
}

void Ui::init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        spdlog::error("Failed to initialize SDL3: {}", SDL_GetError());
        std::exit(-1);
    }
    spdlog::info("Initialized SDL3");

    // GL 3.0 + GLSL 130
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    const auto width = settings.Width();
    const auto height = settings.Height();
    constexpr auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

    window = SDL_CreateWindow(
        "sen - NES Emulator",
        width,
        height,
        window_flags
    );
    if (window == nullptr) {
        spdlog::error("Failed to create SDL3 window: {}", SDL_GetError());
        std::exit(-1);
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        spdlog::error("Failed to create SDL3 OpenGL context: {}", SDL_GetError());
        std::exit(-1);
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(0);

    controller = find_controllers();

    spdlog::info("Initialized SDL3 window and OpenGL context");
}

void Ui::init_sdl_audio() {
    const auto audio_device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &OUTPUT_DEVICE_SPEC);
    if (audio_device_id == 0) {
        spdlog::error("Failed to open SDL audio device: {}", SDL_GetError());
        std::exit(-1);
    }
    audio_queue = std::make_shared<AudioStreamQueue>(audio_device_id);
}

void Ui::init_imgui() const {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
#ifdef WIN32
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
#endif
    spdlog::info("Initialized ImGui context");

    // Icon Fonts
    io.Fonts->AddFontDefault();
    constexpr float baseFontSize = 24.0f;
    constexpr float iconFontSize =
        baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced
    // by 2.0f/3.0f in order to align correctly

    // merge in icons from Font Awesome
    static constexpr ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts
        ->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges);

    set_imgui_style();
    // Setup Dear ImGui style
    switch (settings.GetUiStyle()) {
        case UiStyle::Classic:
            ImGui::StyleColorsClassic();
            break;
        case UiStyle::Light:
            ImGui::StyleColorsLight();
            break;
        case UiStyle::Dark:
            ImGui::StyleColorsDark();
            break;
        case UiStyle::SuperDark:
            EmbraceTheDarkness();
            break;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);
}

SDL_Gamepad* Ui::find_controllers() {
    if (!SDL_HasGamepad()) {
        return nullptr;
    }

    int num_gamepads = 0;
    const auto *const gamepad_ids = SDL_GetGamepads(&num_gamepads);
    if (num_gamepads > 0) {
        return SDL_OpenGamepad(gamepad_ids[0]);
    }
    return nullptr;
}

void Ui::handle_sdl_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                open = false;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    open = false;
                }

            case SDL_EVENT_WINDOW_RESIZED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    settings.Width(event.window.data1);
                    settings.Height(event.window.data2);
                }
                break;

            case SDL_EVENT_GAMEPAD_ADDED:
                if (!controller) {
                    spdlog::info("Controller connected");
                    controller = SDL_OpenGamepad(event.cdevice.which);
                }
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                if (controller && event.cdevice.which == SDL_GetJoystickID(SDL_GetGamepadJoystick(controller))) {
                    spdlog::info("Controller disconnected");
                    SDL_CloseGamepad(controller);
                    controller = find_controllers();
                }
                break;

            default:
                break;
        }
    }

    if (controller == nullptr || !emulation_running) {
        return;
    }

    byte keys{};
    for (auto [controller_key, key] : KEYMAP) {
        if (SDL_GetGamepadButton(controller, controller_key)) {
            keys |= static_cast<byte>(key);
        }
    }
    emulator_context->set_pressed_keys(ControllerPort::Port1, keys);
}

void Ui::set_filter(const FilterType filter) {
    switch (filter) {
        case FilterType::NoFilter:
            this->filter = std::make_unique<NoFilter>();
            break;
        case FilterType::Ntsc:
            this->filter = std::make_unique<NtscFilter>(settings.ScaleFactor());
            break;
    }
}

void Ui::EmbraceTheDarkness() {
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

void Ui::set_imgui_style() {
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

void Ui::run() {
    Uint64 current_time = SDL_GetTicks();

    while (open) {
        const auto new_time = SDL_GetTicks();
        const auto dt = new_time - current_time;
        current_time = new_time;

        const auto cpu_cycles_to_run = dt * CYCLES_PER_FRAME / 16;

        handle_sdl_events();

        if (emulation_running) {
            // Run emulator for `dt` equivalent cycles and check if frame passed
            const auto initial_frame_count = emulator_context->FrameCount();
            emulator_context->RunForCycles(cpu_cycles_to_run);
            if (emulator_context->FrameCount() != initial_frame_count && audio_frame_delay != 0) {
                audio_frame_delay--;
                if (audio_frame_delay == 0) {
                    audio_queue->resume();
                }
            }
        }

        render_ui();

        SDL_GL_SwapWindow(window);
    }
}

Ui::~Ui() {
    try {
        settings.SyncPanelStates();
        settings.cfg.writeFile("test.cfg");
    } catch (const libconfig::FileIOException& e) {
        spdlog::error("Failed to save settings: {}", e.what());
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Ui::render_ui() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

#ifdef IMGUI_HAS_VIEWPORT
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
#else
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    show_menu_bar();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (emulator_context == nullptr) {
        ImGui::Begin("Load ROM", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Load a NES ROM and click on Start to run the program");
        ImGui::End();
    } else {
        ImGui::SetNextWindowSize(ImVec2(NES_WIDTH * DEFAULT_SCALE_FACTOR, 0));
        ImGui::Begin(
            "Game",
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar
        );

        const auto framebuffer = debugger.Framebuffer();
        const auto [data, width, height] = filter->PostProcess(framebuffer, settings.ScaleFactor());

        glBindTexture(GL_TEXTURE_2D, display_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        ImGui::Image(
            display_texture,
            ImVec2(NES_WIDTH * settings.ScaleFactor(), NES_HEIGHT * settings.ScaleFactor())
        );
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::End();

        show_registers();
        show_ppu_memory();
        show_oam();
        show_opcodes();
        show_debugger();
        show_pattern_tables();
    }

    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f); // Disable round borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f); // Disable borders
    ImGui::RenderNotifications();
    ImGui::PopStyleVar(2);

    ImGui::EndFrame();

    ImGui::Render();
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef WIN32
    if (const auto& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
}

void Ui::show_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                nfdu8char_t* outPath;
                constexpr nfdu8filteritem_t filters[1] = {{"NES ROM", "nes"}};
                nfdopendialogu8args_t args = {nullptr};
                args.filterList = filters;
                args.filterCount = 1;

                if (const nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
                    result == NFD_OKAY) {
                    settings.PushRecentPath(outPath);
                    stop_emulation();
                    load_rom_file(outPath);
                    NFD_FreePathU8(outPath);
                } else if (result == NFD_CANCEL) {
                    spdlog::debug("User pressed cancel");
                } else {
                    spdlog::error("Error: {}\n", NFD_GetError());
                }
            }
            if (ImGui::BeginMenu("Open Recent")) {
                static std::vector<const char*> recents;
                settings.RecentRoms(recents);
                for (const auto *recent : recents) {
                    if (ImGui::MenuItem(recent, nullptr, false, true)) {
                        stop_emulation();
                        load_rom_file(recent);
                        ImGui::InsertNotification(
                            {ImGuiToastType::Success, 3000, "Successfully loaded %s", recent}
                        );
                    }
                }

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                open = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation")) {
            if (ImGui::MenuItem(
                    "Start",
                    nullptr,
                    false,
                    !emulation_running && emulator_context != nullptr
                )) {
                start_emulation();
            }
            if (ImGui::MenuItem("Pause", nullptr, false, emulation_running)) {
                pause_emulation();
            }
            if (ImGui::MenuItem("Reset", nullptr, false, emulation_running)) {
                reset_emulation();
            }
            if (ImGui::MenuItem("Stop", nullptr, false, emulation_running)) {
                stop_emulation();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::Text("Framerate: %.2f", ImGui::GetIO().Framerate);
            if (ImGui::BeginMenu("Scale")) {
                for (int i = 1; i <= 5; i++) {
                    if (ImGui::MenuItem(
                            SCALING_FACTORS[i - 1],
                            nullptr,
                            settings.ScaleFactor() == i,
                            emulation_running
                        )) {
                        settings.SetScale(i);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Style")) {
                if (ImGui::MenuItem(
                        "Classic",
                        nullptr,
                        settings.GetUiStyle() == UiStyle::Classic
                    )) {
                    ImGui::StyleColorsClassic();
                    settings.SetUiStyle(UiStyle::Classic);
                }
                if (ImGui::MenuItem("Light", nullptr, settings.GetUiStyle() == UiStyle::Light)) {
                    ImGui::StyleColorsLight();
                    settings.SetUiStyle(UiStyle::Light);
                }
                if (ImGui::MenuItem("Dark", nullptr, settings.GetUiStyle() == UiStyle::Dark)) {
                    ImGui::StyleColorsDark();
                    settings.SetUiStyle(UiStyle::Dark);
                }
                if (ImGui::MenuItem(
                        "Super Dark",
                        nullptr,
                        settings.GetUiStyle() == UiStyle::SuperDark
                    )) {
                    EmbraceTheDarkness();
                    settings.SetUiStyle(UiStyle::SuperDark);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Filter")) {
                if (ImGui::MenuItem(
                        "None",
                        nullptr,
                        settings.GetFilterType() == FilterType::NoFilter
                    )) {
                    settings.SetFilterType(FilterType::NoFilter);
                    set_filter(FilterType::NoFilter);
                }
                if (ImGui::MenuItem(
                        "NTSC",
                        nullptr,
                        settings.GetFilterType() == FilterType::Ntsc
                    )) {
                    settings.SetFilterType(FilterType::Ntsc);
                    set_filter(FilterType::Ntsc);
                }
                ImGui::EndMenu();
            }

            auto open_panels = settings.GetOpenPanels();
            if (ImGui::MenuItem(
                    "Debugger",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::Debugger)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::Debugger);
            }
            if (ImGui::MenuItem(
                    "Registers",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::Registers)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::Registers);
            }
            if (ImGui::MenuItem(
                    "Disassembly",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::Disassembly)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::Disassembly);
            }
            if (ImGui::MenuItem(
                    "Pattern Tables",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::PatternTables)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::PatternTables);
            }
            if (ImGui::MenuItem(
                    "PPU Memory",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::PpuMemory)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::PpuMemory);
            }
            if (ImGui::MenuItem(
                    "Sprites",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::Sprites)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::Sprites);
            }
            if (ImGui::MenuItem(
                    "Volume",
                    nullptr,
                    open_panels[static_cast<int>(UiPanel::VolumeControl)],
                    emulation_running
                )) {
                settings.TogglePanel(UiPanel::VolumeControl);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Ui::show_registers() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Registers)]) {
        return;
    }

    if (ImGui::Begin("Registers", &open_panels[static_cast<int>(UiPanel::Registers)])) {
        ImGui::SeparatorText("CPU Registers");
        const auto [a, x, y, s, pc, p] = debugger.GetCpuState();
        if (ImGui::BeginTable(
                "cpu_registers",
                2,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
            )) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            ImGui::Text("A");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.2X", a);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("X");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.2X", x);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Y");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.2X", y);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("S");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.2X", s);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("PC");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.4X", pc);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("P");
            ImGui::TableNextColumn();

            constexpr ImVec4 gray(0.5f, 0.5f, 0.5f, 1.0f);

            if ((p & static_cast<byte>(StatusFlag::Carry)) != 0) {
                ImGui::Text("C");
            } else {
                ImGui::TextColored(gray, "C");
            }
            ImGui::SameLine();

            if ((p & static_cast<byte>(StatusFlag::Zero)) != 0) {
                ImGui::Text("Z");
            } else {
                ImGui::TextColored(gray, "Z");
            }
            ImGui::SameLine();

            if ((p & static_cast<byte>(StatusFlag::InterruptDisable)) != 0) {
                ImGui::Text("I");
            } else {
                ImGui::TextColored(gray, "I");
            }
            ImGui::SameLine();

            if ((p & static_cast<byte>(StatusFlag::Decimal)) != 0) {
                ImGui::Text("D");
            } else {
                ImGui::TextColored(gray, "D");
            }
            ImGui::SameLine();

            if ((p & static_cast<byte>(StatusFlag::Overflow)) != 0) {
                ImGui::Text("V");
            } else {
                ImGui::TextColored(gray, "V");
            }
            ImGui::SameLine();

            if ((p & static_cast<byte>(StatusFlag::Negative)) != 0) {
                ImGui::Text("N");
            } else {
                ImGui::TextColored(gray, "N");
            }

            ImGui::EndTable();
        }

        ImGui::SeparatorText("PPU Registers");
        static PpuState ppu_state;
        debugger.load_ppu_state(ppu_state);

        if (ImGui::BeginTable(
                "ppu_registers",
                2,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
            )) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            ImGui::Text("Frame Count");
            ImGui::TableNextColumn();
            ImGui::Text("%lu", ppu_state.frame_count);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scanline");
            ImGui::TableNextColumn();
            ImGui::Text("%u", ppu_state.scanline);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scanline Cycles");
            ImGui::TableNextColumn();
            ImGui::Text("%u", ppu_state.line_cycles);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUCTRL");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppu_state.ppuctrl);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUMASK");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppu_state.ppumask);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUSTATUS");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppu_state.ppustatus);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("OAMADDR");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppu_state.oamaddr);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("V");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.4X", ppu_state.v);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("T");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.4X", ppu_state.t);

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void Ui::draw_sprite(
    const size_t index,
    const SpriteData& sprite,
    const std::array<byte, 0x20>& palettes
) const {
    std::vector<Pixel> pixels(8 * 8);
    const auto palette_id = sprite.oam_entry.PaletteIndex() + 4;

    for (int column = 0; column < 8; column++) {
        for (int row = 0; row < 8; row++) {
            const byte low = sprite.tile_data[row];
            const byte high = sprite.tile_data[row + 8];
            const byte pixel_msb = ((high & (1 << column)) != 0) ? 0b10 : 0b00;
            const byte pixel_lsb = ((low & (1 << column)) != 0) ? 0b01 : 0b00;
            const byte color_index = pixel_msb | pixel_lsb;

            const size_t pixel_index = column + row * 8;
            const auto nes_palette_color_index = ((palette_id & 0b111) << 2) | (color_index & 0b11);
            pixels[pixel_index] = PALETTE_COLORS[palettes[nes_palette_color_index]];
        }
    }

    glBindTexture(GL_TEXTURE_2D, sprite_textures[index]);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        8,
        8,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        reinterpret_cast<unsigned char*>(pixels.data())
    );
    ImGui::Image(static_cast<ImTextureID>(sprite_textures[index]), ImVec2(64, 64));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Ui::show_oam() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Sprites)]) {
        return;
    }

    if (ImGui::Begin("Sprites", &open_panels[static_cast<int>(UiPanel::Sprites)])) {
        static Sprites sprites{};
        debugger.load_sprite_data(sprites);
        const auto& sprites_data = sprites.sprites_data;
        const auto& palettes = sprites.palettes;

        if (ImGui::BeginTable("ppu_sprites", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            constexpr ImVec4 gray(0.5f, 0.5f, 0.5f, 1.0f);
            for (size_t i = 0; i < sprites_data.size(); i += 2) {
                auto sprite = sprites_data[i];
                ImGui::TableNextColumn();
                draw_sprite(i, sprite, palettes);
                ImGui::TableNextColumn();
                ImGui::Text("(%d, %d)", sprite.oam_entry.x, sprite.oam_entry.y);
                ImGui::Text("0x%.2X", sprite.oam_entry.tile_index);
                {
                    const auto attribs = sprite.oam_entry.attribs;
                    const bool flip_vertical = (attribs & 0x80) != 0x00;
                    const bool flip_horizontal = (attribs & 0x40) != 0x00;
                    const bool bg_over_sprite = (attribs & 0x20) != 0x00;
                    const auto palette_index = attribs & 0b11;

                    if (flip_vertical) {
                        ImGui::Text("V");
                    } else {
                        ImGui::TextColored(gray, "V");
                    }
                    ImGui::SetItemTooltip("Flip Vertical");
                    ImGui::SameLine();
                    if (flip_horizontal) {
                        ImGui::Text("H");
                    } else {
                        ImGui::TextColored(gray, "H");
                    }
                    ImGui::SetItemTooltip("Flip Horizontal");
                    ImGui::SameLine();
                    if (bg_over_sprite) {
                        ImGui::Text("BG");
                    } else {
                        ImGui::TextColored(gray, "BG");
                    }
                    ImGui::SetItemTooltip("Background over Sprite");
                    ImGui::SameLine();

                    ImGui::Text("%d", palette_index);
                }

                sprite = sprites_data[i + 1];
                ImGui::TableNextColumn();
                draw_sprite(i + 1, sprite, palettes);
                ImGui::TableNextColumn();
                ImGui::Text("(%d, %d)", sprite.oam_entry.x, sprite.oam_entry.y);
                ImGui::Text("0x%.2X", sprite.oam_entry.tile_index);
                {
                    const auto attribs = sprite.oam_entry.attribs;
                    const bool flip_vertical = (attribs & 0x80) != 0x00;
                    const bool flip_horizontal = (attribs & 0x40) != 0x00;
                    const bool bg_over_sprite = (attribs & 0x20) != 0x00;
                    const auto palette_index = attribs & 0b11;

                    if (flip_vertical) {
                        ImGui::Text("V");
                    } else {
                        ImGui::TextColored(gray, "V");
                    }
                    ImGui::SetItemTooltip("Flip Vertical");
                    ImGui::SameLine();
                    if (flip_horizontal) {
                        ImGui::Text("H");
                    } else {
                        ImGui::TextColored(gray, "H");
                    }
                    ImGui::SetItemTooltip("Flip Horizontal");
                    ImGui::SameLine();
                    if (bg_over_sprite) {
                        ImGui::Text("BG");
                    } else {
                        ImGui::TextColored(gray, "BG");
                    }
                    ImGui::SetItemTooltip("Background over Sprite");
                    ImGui::SameLine();

                    ImGui::Text("%d", palette_index);
                }

                ImGui::TableNextRow();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void Ui::show_ppu_memory() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::PpuMemory)]) {
        return;
    }

    if (ImGui::Begin("PPU Memory", &open_panels[static_cast<int>(UiPanel::PpuMemory)])) {
        static std::vector<byte> ppu_memory(0x4000U, 0U);
        debugger.LoadPpuMemory(ppu_memory);
        static MemoryEditor ppu_mem_edit;
        ppu_mem_edit.ReadOnly = true;
        ppu_mem_edit.DrawContents(ppu_memory.data(), 0x4000);
    }
    ImGui::End();
}

void Ui::show_opcodes() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Disassembly)]) {
        return;
    }

    if (ImGui::Begin("Disassembly", &open_panels[static_cast<int>(UiPanel::Disassembly)])) {
        for (const auto executed_opcodes = debugger.GetCpuExecutedOpcodes();
             const auto& executed_opcode : executed_opcodes.values | std::ranges::views::reverse) {
            auto [opcode_class, opcode, addressing_mode, length, cycles, label] =
                OPCODES[executed_opcode.opcode];

            std::string formatted_args;
            ImVec4 arg_color(1.0f, 1.0f, 1.0f, 1.0f);

            switch (addressing_mode) {
                case AddressingMode::Absolute: {
                    assert(length == 3);
                    const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8
                        | static_cast<uint16_t>(executed_opcode.arg1);
                    if (opcode_class == OpcodeClass::JMP || opcode_class == OpcodeClass::JSR) {
                        formatted_args = std::format("0x{:04X}", address);
                    } else {
                        arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                        formatted_args = std::format("(0x{:04X})", address);
                    }

                    break;
                }
                case AddressingMode::AbsoluteXIndexed: {
                    assert(length == 3);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8
                        | static_cast<uint16_t>(executed_opcode.arg1);
                    formatted_args = std::format("(0x{:04X} + X)", address);
                    break;
                }
                case AddressingMode::AbsoluteYIndexed: {
                    assert(length == 3);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8
                        | static_cast<uint16_t>(executed_opcode.arg1);
                    formatted_args = std::format("(0x{:04X} + Y)", address);
                    break;
                }
                case AddressingMode::Immediate:
                    assert(length == 2);
                    arg_color = ImVec4(0.2f, 0.5f, 0.8f, 1.0f);
                    formatted_args = std::format("#0x{:02X}", executed_opcode.arg1);
                    break;
                case AddressingMode::Indirect: {
                    assert(length == 3);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8
                        | static_cast<uint16_t>(executed_opcode.arg1);
                    formatted_args = std::format("(0x{:04X})", address);
                    break;
                }
                case AddressingMode::IndirectX:
                    assert(length == 2);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    formatted_args = std::format("(0x{:02X} + X)", executed_opcode.arg1);
                    break;
                case AddressingMode::IndirectY:
                    assert(length == 2);
                    formatted_args = std::format("(0x{:02X}) + Y", executed_opcode.arg1);
                    break;
                case AddressingMode::Relative:
                    assert(length == 2);
                    formatted_args =
                        std::format("0x{:02X}", static_cast<int8_t>(executed_opcode.arg1));
                    break;
                case AddressingMode::ZeroPage:
                    assert(length == 2);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    formatted_args = std::format("(0x{:04X})", executed_opcode.arg1);
                    break;
                case AddressingMode::ZeroPageX:
                    assert(length == 2);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    formatted_args = std::format("(0x{:04X} + X) % 256", executed_opcode.arg1);
                    break;
                case AddressingMode::ZeroPageY:
                    assert(length == 2);
                    arg_color = ImVec4(0.2f, 0.6f, 0.3f, 1.0f);
                    formatted_args = std::format("(0x{:04X} + Y) % 256", executed_opcode.arg1);
                    break;
                default:
                    break;
            }

            ImGui::TextColored(
                ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "(%lu)    ",
                executed_opcode.start_cycle
            );
            ImGui::SameLine();
            ImGui::Text("0x%.4X    ", executed_opcode.pc);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", label);
            ImGui::SameLine();
            ImGui::TextColored(arg_color, "%s", formatted_args.c_str());
        }
    }
    ImGui::End();
}

void Ui::show_debugger() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Debugger)]) {
        return;
    }

    if (ImGui::Begin("Debugger", &open_panels[static_cast<int>(UiPanel::Debugger)])) {
        if (ImGui::Button(emulation_running ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(30, 30))) {
            emulation_running = !emulation_running;
            if (emulation_running) {
                audio_frame_delay = MAX_AUDIO_FRAME_LAG;
                audio_queue->clear();
            } else {
                audio_queue->resume();
            }
        }
        ImGui::SetItemTooltip("Play/Pause");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STOP, ImVec2(30, 30))) {
            stop_emulation();
        }
        ImGui::SameLine();
        ImGui::SetItemTooltip("Stop");

        if (emulation_running) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT, ImVec2(30, 30))) {
            emulator_context->StepOpcode();
        }
        ImGui::SetItemTooltip("Step");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FORWARD, ImVec2(30, 30))) {
            emulator_context->RunForOneScanline();
        }
        ImGui::SetItemTooltip("Step scanline");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TV, ImVec2(30, 30))) {
            emulator_context->RunForOneFrame();
        }
        ImGui::SetItemTooltip("Step frame");

        if (emulation_running) {
            ImGui::EndDisabled();
        }
    }

    ImGui::End();
}

void Ui::show_volume_control() {
    auto& open_panels = settings.GetOpenPanels();

    if (!open_panels[static_cast<int>(UiPanel::VolumeControl)]) {
        return;
    }

    if (ImGui::Begin("Volume Control", &open_panels[static_cast<int>(UiPanel::VolumeControl)])) {}

    ImGui::End();
}

void Ui::show_pattern_tables() {
    auto& open_panels = settings.GetOpenPanels();

    if (!open_panels[static_cast<int>(UiPanel::PatternTables)]) {
        return;
    }

    if (ImGui::Begin("Pattern Tables", &open_panels[static_cast<int>(UiPanel::PatternTables)])) {
        static PatternTablesState pattern_tables_state;
        debugger.load_pattern_table_state(pattern_tables_state);
        const auto& left = pattern_tables_state.left;
        const auto& right = pattern_tables_state.right;
        const auto& palettes = pattern_tables_state.palettes;

        static int palette_id = 0;
        if (ImGui::InputInt("Palette ID", &palette_id)) {
            palette_id = std::max(std::min(palette_id, 7), 0);
        }

        auto left_pixels = render_pattern_table(left, palettes, palette_id);
        glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            128,
            128,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            left_pixels.data()
        );

        ImGui::Image(static_cast<ImTextureID>(pattern_table_left_texture), ImVec2(385, 385));
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::Separator();

        auto right_pixels = render_pattern_table(right, palettes, palette_id);
        glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            128,
            128,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            reinterpret_cast<unsigned char*>(right_pixels.data())
        );
        ImGui::Image(static_cast<ImTextureID>(pattern_table_right_texture), ImVec2(385, 385));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ImGui::End();
}

void Ui::load_rom_file(const char* path) {
    loaded_rom_file_path = std::make_optional<std::filesystem::path>(path);
    spdlog::info("Loading file {}", loaded_rom_file_path->string());

    const auto rom = ReadBinaryFile(loaded_rom_file_path.value());
    auto rom_args = RomArgs{rom};
    emulator_context = std::make_shared<Sen>(rom_args, audio_queue);
    debugger = Debugger(emulator_context);

    const auto title = fmt::format("Sen - {}", loaded_rom_file_path->filename().string());
    SDL_SetWindowTitle(window, title.c_str());
    audio_frame_delay = MAX_AUDIO_FRAME_LAG;
    audio_queue->clear();
}

std::vector<Pixel> Ui::render_pattern_table(
    const std::array<byte, 0x1000>& pattern_table,
    const std::array<byte, 32>& nes_palette,
    const int palette_id
) {
    static std::vector<Pixel> pixels(128 * 128);

    for (size_t column = 0; column < 128; column++) {
        const size_t tile_column = column / 8;
        const size_t pixel_column_in_tile = column % 8;

        for (size_t row = 0; row < 128; row++) {
            const size_t tile_row = row / 8;
            const size_t pixel_row_in_tile = 7 - (row % 8);

            // TODO: Find out why this works correctly with
            // pixel_{column,row}_in_tile
            const size_t tile_index = tile_row + tile_column * 16;
            const size_t pixel_row_bitplane_0_index = tile_index * 16 + pixel_column_in_tile;
            const size_t pixel_row_bitplane_1_index = pixel_row_bitplane_0_index + 8;

            const byte pixel_row_bitplane_0 = pattern_table[pixel_row_bitplane_0_index];
            const byte pixel_row_bitplane_1 = pattern_table[pixel_row_bitplane_1_index];

            const byte pixel_msb =
                (pixel_row_bitplane_1 & (1 << pixel_row_in_tile)) != 0 ? 0b10 : 0b00;
            const byte pixel_lsb =
                (pixel_row_bitplane_0 & (1 << pixel_row_in_tile)) != 0 ? 0b01 : 0b00;

            const byte color_index = pixel_msb | pixel_lsb;
            const size_t pixel_index = row + column * 128;

            // Skip the first byte for Universal background color
            const auto nes_palette_color_index = ((palette_id & 0b111) << 2) | (color_index & 0b11);

            pixels[pixel_index] = PALETTE_COLORS[nes_palette[nes_palette_color_index]];
        }
    }

    return pixels;
}

void Ui::start_emulation() {
    emulation_running = true;
}

void Ui::pause_emulation() {
    emulation_running = false;
}

void Ui::reset_emulation() {}

void Ui::stop_emulation() {
    emulation_running = false;
    emulator_context = nullptr;
    audio_frame_delay = MAX_AUDIO_FRAME_LAG;
    audio_queue->clear();
}
