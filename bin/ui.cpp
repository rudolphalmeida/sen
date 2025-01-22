#include <span>

#include "ui.hxx"

#include <nfd.h>
#include <SDL2/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "IconsFontAwesome6.h"
#include "ImGuiNotify.hpp"
#include "imgui_memory_editor.h"
#include "util.hxx"


const char* SCALING_FACTORS[] = {"240p (1x)", "480p (2x)", "720p (3x)", "960p (4x)", "1200p (5x)"};
constexpr auto GLSL_VERSION = "#version 130";

void InitTexture(const GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

Ui::Ui() {
    InitSDL();
    InitImGui();
    InitSDLAudio();

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

    SetFilter(settings.GetFilterType());
}

void Ui::InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        spdlog::error("Failed to initialize SDL2: {}", SDL_GetError());
        std::exit(-1);
    }
    spdlog::info("Initialized SDL2");

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
    constexpr auto window_flags =
        SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WindowFlags::SDL_WINDOW_OPENGL;

    window = SDL_CreateWindow("sen - NES Emulator", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, width, height, window_flags);
    if (window == nullptr) {
        spdlog::error("Failed to create SDL2 window: {}", SDL_GetError());
        std::exit(-1);
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        spdlog::error("Failed to create SDL2 OpenGL context: {}", SDL_GetError());
        std::exit(-1);
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync. TODO: Disable vsync

    controller = FindController();

    spdlog::info("Initialized SDL2 window and OpenGL context");
}

void Ui::InitSDLAudio() {
    audio_queue = std::make_shared<AudioStreamQueue>();
    SDL_AudioSpec spec = {
        .freq = DEVICE_SAMPLE_RATE,
        .format = DEVICE_FORMAT,
        .channels = DEVICE_CHANNELS,
        .samples = 2048,
        .callback = [](void * userData, Uint8 * stream, const int length) {
            const auto ui = static_cast<Ui*>(userData);
            ui->audio_queue->GetSamples(stream, length / sizeof(float));
        },
        .userdata = static_cast<void*>(this),
    };

    if (SDL_OpenAudio(&spec, nullptr) < 0) {
        spdlog::error("Failed to initialize audio device: {}", SDL_GetError());
        std::exit(-1);
    }
}

void Ui::InitImGui() const {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
#ifdef WIN32
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport
#endif
    spdlog::info("Initialized ImGui context");

    // Icon Fonts
    io.Fonts->AddFontDefault();
    constexpr float baseFontSize = 24.0f;
    constexpr float iconFontSize =
        baseFontSize * 2.0f / 3.0f;  // FontAwesome fonts need to have their sizes reduced
    // by 2.0f/3.0f in order to align correctly

    // merge in icons from Font Awesome
    static constexpr ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config,
                                 icons_ranges);

    SetImGuiStyle();
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
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);
}

SDL_GameController* Ui::FindController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            spdlog::debug("Found controller with ID: {}", i);
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

void Ui::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                open = false;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                    open = false;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(window)) {
                    settings.Width(event.window.data1);
                    settings.Height(event.window.data2);
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                if (!controller) {
                    spdlog::info("Controller connected");
                    controller = SDL_GameControllerOpen(event.cdevice.which);
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (controller && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                    spdlog::info("Controller disconnected");
                    SDL_GameControllerClose(controller);
                    controller = FindController();
                }
                break;

            case SDL_CONTROLLERBUTTONUP:
                if (emulator_context && emulation_running && controller && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                    if (const auto button = static_cast<SDL_GameControllerButton>(event.cbutton.button); KEYMAP.contains(button)) {
                        emulator_context->ControllerRelease(ControllerPort::Port1, KEYMAP.at(button));
                    }
                }
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                if (emulator_context && emulation_running && controller && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                    if (const auto button = static_cast<SDL_GameControllerButton>(event.cbutton.button); KEYMAP.contains(button)) {
                        emulator_context->ControllerPress(ControllerPort::Port1, KEYMAP.at(button));
                    }                }
                break;

            default: break;
        }

        if (event.type == SDL_QUIT)
            open = false;
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                open = false;
            }
            if (event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(window)) {
                settings.Width(event.window.data1);
                settings.Height(event.window.data2);
            }
        }
    }
}

void Ui::SetFilter(const FilterType filter) {
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

void Ui::SetImGuiStyle() {
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

void Ui::MainLoop() {
    // TODO: UI independent game loop
    while (open) {
        HandleEvents();

        if (emulation_running) {
            if (audio_frame_delay != 0) {
                audio_frame_delay--;
                if (audio_frame_delay == 0) {
                    SDL_PauseAudio(0);
                }
            }
            emulator_context->RunForOneFrame();
        }

        RenderUi();

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
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Ui::RenderUi() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
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

    ShowMenuBar();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (emulator_context == nullptr) {
        ImGui::Begin("Load ROM", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Load a NES ROM and click on Start to run the program");
        ImGui::End();
    } else {
        ImGui::SetNextWindowSize(ImVec2(NES_WIDTH * DEFAULT_SCALE_FACTOR, 0));
        ImGui::Begin(
            "Game", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        const auto framebuffer = debugger.Framebuffer();
        const auto [data, width, height] = filter->PostProcess(framebuffer, settings.ScaleFactor());

        glBindTexture(GL_TEXTURE_2D, display_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     data);
        ImGui::Image(display_texture, ImVec2(NES_WIDTH * settings.ScaleFactor(),
                                             NES_HEIGHT * settings.ScaleFactor()));
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::End();

        ShowRegisters();
        ShowPatternTables();
        ShowPpuMemory();
        ShowOam();
        ShowOpcodes();
        ShowDebugger();
    }

    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f); // Disable round borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f); // Disable borders
    ImGui::RenderNotifications();
    ImGui::PopStyleVar(2);

    ImGui::EndFrame();

    ImGui::Render();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

void Ui::ShowMenuBar() {
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
                    LoadRomFile(outPath);
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
                for (auto recent : recents) {
                    if (ImGui::MenuItem(recent, nullptr, false, true)) {
                        LoadRomFile(recent);
                        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Successfully loaded %s", recent});
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
            if (ImGui::MenuItem("Start", nullptr, false,
                                !emulation_running && emulator_context != nullptr)) {
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
            ImGui::Text("Framerate: %.2f", ImGui::GetIO().Framerate);
            if (ImGui::BeginMenu("Scale")) {
                for (int i = 1; i <= 5; i++) {
                    if (ImGui::MenuItem(SCALING_FACTORS[i - 1], nullptr,
                                        settings.ScaleFactor() == i, emulation_running)) {
                        settings.SetScale(i);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Style")) {
                if (ImGui::MenuItem("Classic", nullptr,
                                    settings.GetUiStyle() == UiStyle::Classic)) {
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
                if (ImGui::MenuItem("Super Dark", nullptr,
                                    settings.GetUiStyle() == UiStyle::SuperDark)) {
                    EmbraceTheDarkness();
                    settings.SetUiStyle(UiStyle::SuperDark);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Filter")) {
                if (ImGui::MenuItem("None", nullptr, settings.GetFilterType() == FilterType::NoFilter)) {
                    settings.SetFilterType(FilterType::NoFilter);
                    SetFilter(FilterType::NoFilter);
                }
                if (ImGui::MenuItem("NTSC", nullptr, settings.GetFilterType() == FilterType::Ntsc)) {
                    settings.SetFilterType(FilterType::Ntsc);
                    SetFilter(FilterType::Ntsc);
                }
                ImGui::EndMenu();
            }

            auto open_panels = settings.GetOpenPanels();
            if (ImGui::MenuItem("Debugger", nullptr,
                                open_panels[static_cast<int>(UiPanel::Debugger)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::Debugger);
            }
            if (ImGui::MenuItem("Registers", nullptr,
                                open_panels[static_cast<int>(UiPanel::Registers)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::Registers);
            }
            if (ImGui::MenuItem("Opcodes", nullptr, open_panels[static_cast<int>(UiPanel::Opcodes)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::Opcodes);
            }
            if (ImGui::MenuItem("Pattern Tables", nullptr,
                                open_panels[static_cast<int>(UiPanel::PatternTables)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::PatternTables);
            }
            if (ImGui::MenuItem("PPU Memory", nullptr,
                                open_panels[static_cast<int>(UiPanel::PpuMemory)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::PpuMemory);
            }
            if (ImGui::MenuItem("Sprites", nullptr, open_panels[static_cast<int>(UiPanel::Sprites)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::Sprites);
            }
            if (ImGui::MenuItem("Volume", nullptr, open_panels[static_cast<int>(UiPanel::VolumeControl)],
                                emulation_running)) {
                settings.TogglePanel(UiPanel::VolumeControl);
                                }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Ui::ShowRegisters() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Registers)]) {
        return;
    }

    if (ImGui::Begin("Registers", &open_panels[static_cast<int>(UiPanel::Registers)])) {
        ImGui::SeparatorText("CPU Registers");
        const auto [a, x, y, s, pc, p] = debugger.GetCpuState();
        if (ImGui::BeginTable("cpu_registers", 2,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
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
        const auto [palettes, frame_count, scanline, line_cycles, v, t, ppuctrl, ppumask, ppustatus,
                    oamaddr] = debugger.GetPpuState();
        if (ImGui::BeginTable("ppu_registers", 2,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            ImGui::Text("Frame Count");
            ImGui::TableNextColumn();
            ImGui::Text("%lu", frame_count);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scanline");
            ImGui::TableNextColumn();
            ImGui::Text("%u", scanline);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scanline Cycles");
            ImGui::TableNextColumn();
            ImGui::Text("%u", line_cycles);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUCTRL");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppuctrl);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUMASK");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppumask);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("PPUSTATUS");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", ppustatus);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("OAMADDR");
            ImGui::TableNextColumn();
            ImGui::Text("%.8b", oamaddr);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("V");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.4X", v);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("T");
            ImGui::TableNextColumn();
            ImGui::Text("0x%.4X", t);

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void Ui::DrawSprite(const size_t index,
                    const SpriteData& sprite,
                    const std::span<byte, 0x20>& palettes) const {
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<unsigned char*>(pixels.data()));
    ImGui::Image(static_cast<ImTextureID>(sprite_textures[index]), ImVec2(64, 64));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Ui::ShowOam() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Sprites)]) {
        return;
    }

    if (ImGui::Begin("Sprites", &open_panels[static_cast<int>(UiPanel::Sprites)])) {
        const auto [sprites_data, palettes] = debugger.GetSprites();

        if (ImGui::BeginTable("ppu_sprites", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            constexpr ImVec4 gray(0.5f, 0.5f, 0.5f, 1.0f);
            for (size_t i = 0; i < sprites_data.size(); i += 2) {
                auto sprite = sprites_data[i];
                ImGui::TableNextColumn();
                DrawSprite(i, sprite, palettes);
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
                DrawSprite(i + 1, sprite, palettes);
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

void Ui::ShowPpuMemory() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::PpuMemory)]) {
        return;
    }

    if (ImGui::Begin("PPU Memory", &open_panels[static_cast<int>(UiPanel::PpuMemory)])) {
        static std::vector<byte> ppu_memory;
        ppu_memory.resize(0x4000);
        debugger.LoadPpuMemory(ppu_memory);
        static MemoryEditor ppu_mem_edit;
        ppu_mem_edit.ReadOnly = true;
        ppu_mem_edit.DrawContents(ppu_memory.data(), 0x4000);
    }
    ImGui::End();
}

void Ui::ShowOpcodes() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Opcodes)]) {
        return;
    }

    if (ImGui::Begin("Opcodes", &open_panels[static_cast<int>(UiPanel::Opcodes)])) {
        if (ImGui::BeginTable("opcodes", 3)) {
            ImGui::TableSetupColumn("Start cycle");
            ImGui::TableSetupColumn("Program Counter");
            ImGui::TableSetupColumn("Disassembly");
            ImGui::TableHeadersRow();

            for (const auto executed_opcodes = debugger.GetCpuExecutedOpcodes();
                 const auto& executed_opcode :
                 executed_opcodes.values | std::ranges::views::reverse) {
                auto [opcode_class, opcode, addressing_mode, length, cycles, label] =
                    OPCODES[executed_opcode.opcode];

                std::string formatted_args;
                switch (addressing_mode) {
                    case AddressingMode::Absolute: {
                        assert(length == 3);
                        const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8 |
                                                 static_cast<uint16_t>(executed_opcode.arg1);
                        if (opcode_class == OpcodeClass::JMP || opcode_class == OpcodeClass::JSR) {
                            formatted_args = std::format("0x{:04X}", address);
                        } else {
                            formatted_args = std::format("(0x{:04X})", address);
                        }

                        break;
                    }
                    case AddressingMode::AbsoluteXIndexed: {
                        assert(length == 3);
                        const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8 |
                                                 static_cast<uint16_t>(executed_opcode.arg1);
                        formatted_args = std::format("(0x{:04X} + X)", address);
                        break;
                    }
                    case AddressingMode::AbsoluteYIndexed: {
                        assert(length == 3);
                        const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8 |
                                                 static_cast<uint16_t>(executed_opcode.arg1);
                        formatted_args = std::format("(0x{:04X} + Y)", address);
                        break;
                    }
                    case AddressingMode::Immediate:
                        assert(length == 2);
                        formatted_args = std::format("#0x{:02X}", executed_opcode.arg1);
                        break;
                    case AddressingMode::Indirect: {
                        assert(length == 3);
                        const uint16_t address = static_cast<uint16_t>(executed_opcode.arg2) << 8 |
                                                 static_cast<uint16_t>(executed_opcode.arg1);
                        formatted_args = std::format("(0x{:04X})", address);
                        break;
                    }
                    case AddressingMode::IndirectX:
                        assert(length == 2);
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
                        formatted_args = std::format("(0x{:04X})", executed_opcode.arg1);
                        break;
                    case AddressingMode::ZeroPageX:
                        assert(length == 2);
                        formatted_args = std::format("(0x{:04X} + X) % 256", executed_opcode.arg1);
                        break;
                    case AddressingMode::ZeroPageY:
                        assert(length == 2);
                        formatted_args = std::format("(0x{:04X} + Y) % 256", executed_opcode.arg1);
                        break;
                    default:
                        break;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%lu", executed_opcode.start_cycle);
                ImGui::TableNextColumn();
                ImGui::Text("0x%.4X", executed_opcode.pc);
                ImGui::TableNextColumn();
                ImGui::Text("%s %s", label, formatted_args.c_str());
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void Ui::ShowDebugger() {
    auto& open_panels = settings.GetOpenPanels();
    if (!open_panels[static_cast<int>(UiPanel::Debugger)]) {
        return;
    }

    if (ImGui::Begin("Debugger", &open_panels[static_cast<int>(UiPanel::Debugger)])) {
        if (ImGui::Button(emulation_running ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(30, 30))) {
            emulation_running = !emulation_running;
            if (emulation_running) {
                audio_frame_delay = MAX_AUDIO_FRAME_LAG;
                audio_queue->Clear();
                SDL_PauseAudio(0);
            } else {
                SDL_PauseAudio(1);
            }
        }
        ImGui::SetItemTooltip("Play/Pause");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STOP, ImVec2(30, 30))) {
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
void Ui::ShowVolumeControl() {
    auto& open_panels = settings.GetOpenPanels();

    if (!open_panels[static_cast<int>(UiPanel::VolumeControl)]) {
        return;
    }

    if (ImGui::Begin("Volume Control", &open_panels[static_cast<int>(UiPanel::VolumeControl)])) {

    }

    ImGui::End();
}

void Ui::ShowPatternTables() {
    auto& open_panels = settings.GetOpenPanels();

    if (!open_panels[static_cast<int>(UiPanel::PatternTables)]) {
        return;
    }

    if (ImGui::Begin("Pattern Tables", &open_panels[static_cast<int>(UiPanel::PatternTables)])) {
        auto [left, right, palettes] = debugger.GetPatternTableState();
        static int palette_id = 0;
        if (ImGui::InputInt("Palette ID", &palette_id)) {
            if (palette_id < 0) {
                palette_id = 0;
            }

            if (palette_id > 7) {
                palette_id = 7;
            }
        }

        auto left_pixels = RenderPixelsForPatternTable(left, palettes, palette_id);
        glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     reinterpret_cast<unsigned char*>(left_pixels.data()));

        ImGui::Image(static_cast<ImTextureID>(pattern_table_left_texture), ImVec2(385, 385));
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::Separator();

        auto right_pixels = RenderPixelsForPatternTable(right, palettes, palette_id);
        glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     reinterpret_cast<unsigned char*>(right_pixels.data()));
        ImGui::Image(static_cast<ImTextureID>(pattern_table_right_texture), ImVec2(385, 385));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ImGui::End();
}

void Ui::LoadRomFile(const char* path) {
    loaded_rom_file_path = std::make_optional<std::filesystem::path>(path);
    spdlog::info("Loading file {}", loaded_rom_file_path->string());

    const auto rom = ReadBinaryFile(loaded_rom_file_path.value());
    auto rom_args = RomArgs{rom};
    emulator_context = std::make_shared<Sen>(rom_args, audio_queue);
    debugger = Debugger(emulator_context);

    const auto title = fmt::format("Sen - {}", loaded_rom_file_path->filename().string());
    SDL_SetWindowTitle(window, title.c_str());
    audio_frame_delay = MAX_AUDIO_FRAME_LAG;
    audio_queue->Clear();
}

std::vector<Pixel> Ui::RenderPixelsForPatternTable(const std::span<byte, 4096> pattern_table,
                                                   const std::span<byte, 32> nes_palette,
                                                   const int palette_id) {
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

void Ui::StartEmulation() {
    emulation_running = true;
}

void Ui::PauseEmulation() {
    emulation_running = false;
}

void Ui::ResetEmulation() {}

void Ui::StopEmulation() {}
