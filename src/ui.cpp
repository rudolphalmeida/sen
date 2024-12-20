#include <span>

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <imgui.h>
#include <nfd.h>
#include <spdlog/spdlog.h>
#include <libconfig.h++>

#include "IconsFontAwesome5.h"
#include "imgui_memory_editor.h"

#include "constants.hxx"
#include "cpu.hxx"
#include "ui.hxx"

#define DEVICE_FORMAT ma_format_u8
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000

const char* SCALING_FACTORS[] = {"240p (1x)", "480p (2x)", "720p (3x)", "960p (4x)", "1200p (5x)"};

static void glfw_error_callback(int error, const char* description) {
    spdlog::error("GLFW error ({}): {}", error, description);
}

Ui::Ui() {
    try {
        // TODO: Change this path to be the standard config directory for each OS
        settings.cfg.readFile("test.cfg");
    } catch (const libconfig::FileIOException&) {
        spdlog::info("Failed to find settings. Using default");
    } catch (const libconfig::ParseException& e) {
        spdlog::error("Failed to parse settings in file {} with {}. Using default", e.getFile(),
                      e.getLine());
    }

    libconfig::Setting& root = settings.cfg.getRoot();
    if (!root.exists("ui")) {
        root.add("ui", libconfig::Setting::TypeGroup);
    }
    libconfig::Setting& ui_settings = root["ui"];
    int scale_factor = DEFAULT_SCALE_FACTOR;
    if (!ui_settings.exists("scale")) {
        ui_settings.add("scale", libconfig::Setting::TypeInt) = DEFAULT_SCALE_FACTOR;
    } else {
        scale_factor = settings.ScaleFactor();
    }
    if (!ui_settings.exists("recents")) {
        ui_settings.add("recents", libconfig::Setting::TypeArray);
    }
    if (!ui_settings.exists("style")) {
        ui_settings.add("style", libconfig::Setting::TypeInt) = static_cast<int>(UiStyle::Dark);
    }
    if (!ui_settings.exists("filter")) {
        ui_settings.add("filter", libconfig::Setting::TypeInt) = static_cast<int>(FilterType::NoFilter);
    }
    if (!ui_settings.exists("open_panels")) {
        ui_settings.add("open_panels", libconfig::Setting::TypeInt) = 0;
    } else {
        const auto saved_open_panels = settings.GetOpenPanels();
        for (int i = 0; i < NUM_PANELS; i++) {
            open_panels[i] = (saved_open_panels & (1 << i)) != 0;
        }
    }
    int width = NES_WIDTH * DEFAULT_SCALE_FACTOR + 15;
    int height = NES_HEIGHT * DEFAULT_SCALE_FACTOR + 55;
    if (!ui_settings.exists("width")) {
        ui_settings.add("width", libconfig::Setting::TypeInt) = width;
    } else {
        width = settings.Width();
    }
    if (!ui_settings.exists("height")) {
        ui_settings.add("height", libconfig::Setting::TypeInt) = height;
    } else {
        height = settings.Height();
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::exit(-1);
    }
    spdlog::info("Initialized GLFW");

    // GL 3.2 + GLSL 150
    const auto glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#endif

    window = glfwCreateWindow(width, height, "sen - NES Emulator", nullptr, nullptr);
    if (window == nullptr) {
        spdlog::error("Failed to create GLFW3 window");
        std::exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    glfwSetWindowUserPointer(window, static_cast<void*>(this));
    glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, const int _width, const int _height) {
        const auto self = static_cast<Ui*>(glfwGetWindowUserPointer(_window));
        self->settings.Width(_width);
        self->settings.Height(_height);
        glViewport(0, 0, _width, _height);
    });

    spdlog::info("Initialized GLFW window and OpenGL context");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
#ifdef WIN32
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport /
                                                         // Platform Windows
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
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup OpenGL textures for drawing
    glGenTextures(1, &pattern_table_left_texture);
    glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &pattern_table_right_texture);
    glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &display_texture);
    glBindTexture(GL_TEXTURE_2D, display_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(64, sprite_textures.data());
    for (int i = 0; i < 64; i++) {
        glBindTexture(GL_TEXTURE_2D, sprite_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    SetFilter(settings.GetFilterType());

    // ma_device_config deviceConfig;
    //
    // deviceConfig = ma_device_config_init(ma_device_type_playback);
    // deviceConfig.playback.format = DEVICE_FORMAT;
    // deviceConfig.playback.channels = DEVICE_CHANNELS;
    // deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    // deviceConfig.dataCallback = [](ma_device* device, void* output, const void*,
    //                                ma_uint32 frame_count) {
    //     const auto ui = static_cast<Ui*>(device->pUserData);
    //     ui->RunForSamples(frame_count);
    //     memset(output, 0x00, frame_count * 2);
    // };
    // deviceConfig.pUserData = this;
    //
    // if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
    //     spdlog::error("Failed to open playback device.\n");
    //     std::exit(-4);
    // }
    //
    // if (ma_device_start(&device) != MA_SUCCESS) {
    //     spdlog::error("Failed to start playback device.\n");
    //     ma_device_uninit(&device);
    //     std::exit(-5);
    // }
}
void Ui::RunForSamples(uint32_t cycles) {
    if (!emulator_context || !emulation_running) {
        return;
    }

    spdlog::debug("Run for {}", cycles);
}

void Ui::HandleInput() const {
    if (!emulator_context || !emulation_running) {
        return;
    }

    for (auto [key, controller_key] : KEYMAP) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            emulator_context->ControllerPress(ControllerPort::Port1, controller_key);
        }
    }
}

void Ui::MainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        HandleInput();

        if (emulation_running) {
            emulator_context->RunForOneFrame();
        }

        RenderUi();

        glfwSwapBuffers(window);
    }
}

void Ui::RenderUi() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
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

    ImGui::EndFrame();

    ImGui::Render();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef WIN32
    if (const auto& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
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
                for (const auto& recent : recents) {
                    if (ImGui::MenuItem(recent, nullptr, false, true)) {
                        LoadRomFile(recent);
                    }
                }

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                glfwSetWindowShouldClose(window, true);
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

            if (ImGui::MenuItem("Debugger", nullptr,
                                open_panels[static_cast<int>(UiPanel::Debugger)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::Debugger)] =
                    !open_panels[static_cast<int>(UiPanel::Debugger)];
            }
            if (ImGui::MenuItem("Registers", nullptr,
                                open_panels[static_cast<int>(UiPanel::Registers)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::Registers)] =
                    !open_panels[static_cast<int>(UiPanel::Registers)];
            }
            if (ImGui::MenuItem("Opcodes", nullptr, open_panels[static_cast<int>(UiPanel::Opcodes)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::Opcodes)] =
                    !open_panels[static_cast<int>(UiPanel::Opcodes)];
            }
            if (ImGui::MenuItem("Pattern Tables", nullptr,
                                open_panels[static_cast<int>(UiPanel::PatternTables)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::PatternTables)] =
                    !open_panels[static_cast<int>(UiPanel::PatternTables)];
            }
            if (ImGui::MenuItem("PPU Memory", nullptr,
                                open_panels[static_cast<int>(UiPanel::PpuMemory)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::PpuMemory)] =
                    !open_panels[static_cast<int>(UiPanel::PpuMemory)];
            }
            if (ImGui::MenuItem("Sprites", nullptr, open_panels[static_cast<int>(UiPanel::Sprites)],
                                emulation_running)) {
                open_panels[static_cast<int>(UiPanel::Sprites)] =
                    !open_panels[static_cast<int>(UiPanel::Sprites)];
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Ui::ShowRegisters() {
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
                    const auto palette_index = attribs & 0b11;
                    const bool bg_over_sprite = (attribs & 0x20) != 0x00;
                    const bool flip_horizontal = (attribs & 0x40) != 0x00;
                    const bool flip_vertical = (attribs & 0x80) != 0x00;

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
                    const auto palette_index = attribs & 0b11;
                    const bool bg_over_sprite = (attribs & 0x20) != 0x00;
                    const bool flip_horizontal = (attribs & 0x40) != 0x00;
                    const bool flip_vertical = (attribs & 0x80) != 0x00;

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
    if (!open_panels[static_cast<int>(UiPanel::Debugger)]) {
        return;
    }

    if (ImGui::Begin("Debugger", &open_panels[static_cast<int>(UiPanel::Debugger)])) {
        if (ImGui::Button(emulation_running ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(30, 30))) {
            emulation_running = !emulation_running;
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

        if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT, ImVec2(30, 30))) {
            emulator_context->StepOpcode();
        }
        ImGui::SetItemTooltip("Step");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STEP_FORWARD, ImVec2(30, 30))) {
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

void Ui::ShowPatternTables() {
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

std::vector<Pixel> Ui::RenderPixelsForPatternTable(const std::span<byte, 4096> pattern_table,
                                                   const std::span<byte, 32> nes_palette,
                                                   const int palette_id) {
    std::vector<Pixel> pixels(128 * 128);

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
