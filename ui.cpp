#include <span>

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <imgui.h>
#include <nfd.h>
#include <spdlog/spdlog.h>
#include <libconfig.h++>

#include "imgui_memory_editor.h"
#include "stb_image_write.h"

#include "constants.hxx"
#include "cpu.hxx"
#include "sen.hxx"
#include "ui.hxx"
#include "util.hxx"

const char* SCALING_FACTORS[] = {"240p (1x)", "480p (2x)", "720p (3x)", "960p (4x)", "1200p (5x)"};

static void glfw_error_callback(int error, const char* description) {
    spdlog::error("GLFW error ({}): {}", error, description);
}

Ui::Ui() {
    try {
        // TODO: Change this path to be the standard config directory for each OS
        settings.cfg.readFile("test.cfg");
    } catch (const libconfig::FileIOException& e) {
        spdlog::info("Failed to find settings. Using default");
    } catch (const libconfig::ParseException& e) {
        spdlog::error("Failed to parse settings in file {} with {}. Using default", e.getFile(), e.getLine());
    }

    libconfig::Setting& root = settings.cfg.getRoot();
    if (!root.exists("ui")) {
        root.add("ui", libconfig::Setting::TypeGroup);
    }
    libconfig::Setting& ui = root["ui"];
    if (!ui.exists("scale")) {
        ui.add("scale", libconfig::Setting::TypeInt) = DEFAULT_SCALE_FACTOR;
    }
    if (!ui.exists("recents")) {
        ui.add("recents", libconfig::Setting::TypeArray);
    }
    if (!ui.exists("style")) {
        ui.add("style", libconfig::Setting::TypeInt) = static_cast<int>(UiStyle::Dark);
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::exit(-1);
    }
    spdlog::info("Initialized GLFW");

    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(NES_WIDTH * DEFAULT_SCALE_FACTOR + 405, NES_HEIGHT * DEFAULT_SCALE_FACTOR + 50,
                              "sen - NES Emulator", nullptr, nullptr);
    if (window == nullptr) {
        spdlog::error("Failed to create GLFW3 window");
        std::exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync
    spdlog::info("Initialized GLFW window and OpenGL context");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    spdlog::info("Initialized ImGui context");

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
}

void Ui::Run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (emulation_running) {
            emulator_context->RunForOneFrame();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#ifdef IMGUI_HAS_VIEWPORT
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
#else
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin("sen", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
            ShowMenuBar();

            {
                ImGui::BeginChild("left-debug-pane", ImVec2(400, 0), true);

                if (ImGui::BeginTabBar("debug_tabs", ImGuiTabBarFlags_None)) {
                    if (ImGui::BeginTabItem("CPU")) {
                        ShowCpuState();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("PPU")) {
                        ShowPpuState();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Pattern Tables")) {
                        ShowPatternTables();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("CPU Memory")) {
                        ShowCpuMemory();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("PPU Memory")) {
                        ShowPpuMemory();
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Cartridge Info")) {
                        ShowCartInfo();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndChild();
            }
            ImGui::SameLine();

            {
                ImGui::BeginChild("right-game-pane", ImVec2(NES_WIDTH * DEFAULT_SCALE_FACTOR, 0), true);

                if (emulator_context != nullptr) {
                    auto framebuffer = debugger.Framebuffer();
                    std::vector<Pixel> pixels(NES_WIDTH * NES_HEIGHT);
                    for (int y = 0; y < NES_HEIGHT; y++) {
                        for (int x = 0; x < NES_WIDTH; x++) {
                            byte color_index = framebuffer[y * NES_WIDTH + x];
                            pixels[y * NES_WIDTH + x] = PALETTE_COLORS[color_index];
                        }
                    }

                    if (ImGui::Button("Take screenshot")) {
                        stbi_write_png("./debug.png", NES_WIDTH, NES_HEIGHT, 3, static_cast<void *>(pixels.data()), 0);
                    }

                    glBindTexture(GL_TEXTURE_2D, display_texture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, NES_WIDTH, NES_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE,
                                 reinterpret_cast<unsigned char*>(pixels.data()));
                    ImGui::Image((void*)(intptr_t)display_texture,
                                 ImVec2(NES_WIDTH * settings.ScaleFactor(), NES_HEIGHT * settings.ScaleFactor()));
                    glBindTexture(GL_TEXTURE_2D, 0);
                } else {
                    ImGui::Text("Load a NES ROM and click on Start to run the program");
                }

                ImGui::EndChild();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(1);

        ImGui::EndFrame();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // TODO: Enable this when I figure out how to get multi viewports to work
        //        auto& io = ImGui::GetIO();
        //        // Update and Render additional Platform Windows
        //        // (Platform functions may change the current OpenGL context, so we save/restore
        //        it to make
        //        // it easier to paste this code elsewhere.
        //        //  For this specific demo app we could also call glfwMakeContextCurrent(window)
        //        directly) if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //            GLFWwindow* backup_current_context = glfwGetCurrentContext();
        //            ImGui::UpdatePlatformWindows();
        //            ImGui::RenderPlatformWindowsDefault();
        //            glfwMakeContextCurrent(backup_current_context);
        //        }

        glfwSwapBuffers(window);
    }
}

void Ui::ShowMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                nfdchar_t* selected_path = nullptr;
                nfdresult_t result = NFD_OpenDialog("nes", nullptr, &selected_path);

                if (result == NFD_OKAY) {
                    settings.PushRecentPath(selected_path);
                    LoadRomFile(selected_path);
                    free(selected_path);
                } else if (result == NFD_CANCEL) {
                    spdlog::debug("User pressed cancel");
                } else {
                    spdlog::error("Error: {}\n", NFD_GetError());
                }
            }
            if (ImGui::BeginMenu("Open Recent")) {
                static std::vector<const char *> recents;
                settings.RecentRoms(recents);
                for (const auto& recent: recents) {
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
            if (ImGui::BeginMenu("Scale")) {
                for (int i = 1; i < 5; i++) {
                    if (ImGui::MenuItem(fmt::format("{}", i).c_str(), nullptr, settings.ScaleFactor() == i, emulation_running)) {
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
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Ui::ShowCpuState() {
    if (emulator_context == nullptr) {
        ImGui::Text("Load a ROM to view NES CPU state");
        return;
    }

    auto cpu_state = debugger.GetCpuState();
    if (ImGui::BeginTable("cpu_registers", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Register");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextColumn();
        ImGui::Text("A");
        ImGui::TableNextColumn();
        ImGui::Text("0x%.2X", cpu_state.a);

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("X");
        ImGui::TableNextColumn();
        ImGui::Text("0x%.2X", cpu_state.x);

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("Y");
        ImGui::TableNextColumn();
        ImGui::Text("0x%.2X", cpu_state.y);

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("S");
        ImGui::TableNextColumn();
        ImGui::Text("0x%.2X", cpu_state.s);

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("PC");
        ImGui::TableNextColumn();
        ImGui::Text("0x%.4X", cpu_state.pc);

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("P");
        ImGui::TableNextColumn();

        ImVec4 gray(0.5f, 0.5f, 0.5f, 1.0f);

        if ((cpu_state.p & static_cast<byte>(StatusFlag::Carry)) != 0) {
            ImGui::Text("C");
        } else {
            ImGui::TextColored(gray, "C");
        }
        ImGui::SameLine();

        if ((cpu_state.p & static_cast<byte>(StatusFlag::Zero)) != 0) {
            ImGui::Text("Z");
        } else {
            ImGui::TextColored(gray, "Z");
        }
        ImGui::SameLine();

        if ((cpu_state.p & static_cast<byte>(StatusFlag::InterruptDisable)) != 0) {
            ImGui::Text("I");
        } else {
            ImGui::TextColored(gray, "I");
        }
        ImGui::SameLine();

        if ((cpu_state.p & static_cast<byte>(StatusFlag::Decimal)) != 0) {
            ImGui::Text("D");
        } else {
            ImGui::TextColored(gray, "D");
        }
        ImGui::SameLine();

        if ((cpu_state.p & static_cast<byte>(StatusFlag::Overflow)) != 0) {
            ImGui::Text("V");
        } else {
            ImGui::TextColored(gray, "V");
        }
        ImGui::SameLine();

        if ((cpu_state.p & static_cast<byte>(StatusFlag::Negative)) != 0) {
            ImGui::Text("N");
        } else {
            ImGui::TextColored(gray, "N");
        }

        ImGui::EndTable();
    }

    for (auto& executed_opcode : cpu_state.executed_opcodes.values) {
        Opcode opcode = OPCODES[executed_opcode.opcode];
        switch (opcode.length) {
            case 1:
                ImGui::Text("0x%.4X: %s", executed_opcode.pc, opcode.label);
                break;
            case 2:
                ImGui::Text("0x%.4X: %s 0x%.2X", executed_opcode.pc, opcode.label,
                            executed_opcode.arg1);
                break;
            case 3:
                ImGui::Text("0x%.4X: %s 0x%.2X 0x%.2X", executed_opcode.pc, opcode.label,
                            executed_opcode.arg1, executed_opcode.arg2);
                break;
        }
    }
}

void Ui::ShowPpuState() {
    if (emulator_context == nullptr) {
        ImGui::Text("Load a ROM to view NES PPU state");
        return;
    }

    auto ppu_state = debugger.GetPpuState();
    if (ImGui::BeginTable("ppu_registers", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Register");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

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
        ImGui::TableNextRow();

        ImGui::EndTable();
    }
}

void Ui::ShowPpuMemory() {
    if (emulator_context == nullptr) {
        ImGui::Text("Load a ROM to view its memory");
        return;
    }

    static std::vector<byte> ppu_memory;
    ppu_memory.resize(0x4000);
    debugger.LoadPpuMemory(ppu_memory);
    static MemoryEditor ppu_mem_edit;
    ppu_mem_edit.ReadOnly = true;
    ppu_mem_edit.DrawContents(ppu_memory.data(), 0x4000);
}

void Ui::ShowPatternTables() {
    if (emulator_context == nullptr) {
        ImGui::Text("Load a ROM to view its pattern tables");
        return;
    }
    auto pattern_table_state = debugger.GetPatternTableState();
    static int palette_id = 0;
    if (ImGui::InputInt("Palette ID", &palette_id)) {
        if (palette_id < 0) {
            palette_id = 0;
        }

        if (palette_id > 7) {
            palette_id = 7;
        }
    }

    auto left_pixels = RenderPixelsForPatternTable(pattern_table_state.left,
                                                   pattern_table_state.palettes, palette_id);
    glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<unsigned char*>(left_pixels.data()));

    ImGui::Image((void*)(intptr_t)pattern_table_left_texture, ImVec2(385, 385));
    glBindTexture(GL_TEXTURE_2D, 0);

    ImGui::Separator();

    auto right_pixels = RenderPixelsForPatternTable(pattern_table_state.right,
                                                    pattern_table_state.palettes, palette_id);
    glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<unsigned char*>(right_pixels.data()));
    ImGui::Image((void*)(intptr_t)pattern_table_right_texture, ImVec2(385, 385));
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::vector<Pixel> Ui::RenderPixelsForPatternTable(std::span<byte, 4096> pattern_table,
                                                   std::span<byte, 32> nes_palette,
                                                   int palette_id) {
    std::vector<Pixel> pixels(128 * 128);

    for (size_t column = 0; column < 128; column++) {
        size_t tile_column = column / 8;
        size_t pixel_column_in_tile = column % 8;

        for (size_t row = 0; row < 128; row++) {
            size_t tile_row = row / 8;
            size_t pixel_row_in_tile = 7 - (row % 8);

            // TODO: Find out why this works correctly with
            // pixel_{column,row}_in_tile
            size_t tile_index = tile_row + tile_column * 16;
            size_t pixel_row_bitplane_0_index = tile_index * 16 + pixel_column_in_tile;
            size_t pixel_row_bitplane_1_index = pixel_row_bitplane_0_index + 8;

            byte pixel_row_bitplane_0 = pattern_table[pixel_row_bitplane_0_index];
            byte pixel_row_bitplane_1 = pattern_table[pixel_row_bitplane_1_index];

            byte pixel_msb = (pixel_row_bitplane_1 & (1 << pixel_row_in_tile)) != 0 ? 0b10 : 0b00;
            byte pixel_lsb = (pixel_row_bitplane_0 & (1 << pixel_row_in_tile)) != 0 ? 0b01 : 0b00;

            byte color_index = pixel_msb | pixel_lsb;
            size_t pixel_index = row + column * 128;

            // Skip the first byte for Universal background color
            auto nes_palette_color_index = ((palette_id & 0b111) << 2) | (color_index & 0b11);

            pixels[pixel_index] = PALETTE_COLORS[nes_palette[nes_palette_color_index + 1]];
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
