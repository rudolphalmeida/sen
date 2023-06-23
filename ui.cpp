#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <imgui.h>
#include <nfd.h>
#include <spdlog/spdlog.h>

#include "imgui_memory_editor.h"

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

    window = glfwCreateWindow(NES_WIDTH * scale_factor + 405, NES_HEIGHT * scale_factor,
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
    ImGui::StyleColorsDark();
    spdlog::info("Using ImGui dark style");

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

        auto& io = ImGui::GetIO();
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        if (ImGui::Begin("sen", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoTitleBar)) {
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
                ImGui::BeginChild("right-game-pane", ImVec2(NES_WIDTH * 4, 0), true);

                if (emulator_context != nullptr) {
                } else {
                    ImGui::Text("Load a NES ROM and click on Start to run the program");
                }

                ImGui::EndChild();
            }
        }
        ImGui::End();

        ImGui::EndFrame();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void Ui::ShowMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                nfdchar_t* selected_path = NULL;
                nfdresult_t result = NFD_OpenDialog("nes", NULL, &selected_path);

                if (result == NFD_OKAY) {
                    loaded_rom_file_path = std::make_optional<std::filesystem::path>(selected_path);
                    free(selected_path);
                    spdlog::info("Loading file {}", loaded_rom_file_path->string());

                    auto rom = read_binary_file(loaded_rom_file_path.value());
                    auto rom_args = RomArgs{rom};
                    emulator_context = std::make_shared<Sen>(rom_args);
                    debugger = Debugger(emulator_context);

                    auto title = fmt::format("Sen - {}", loaded_rom_file_path->filename().string());
                    glfwSetWindowTitle(window, title.c_str());
                } else if (result == NFD_CANCEL) {
                    spdlog::debug("User pressed cancel");
                } else {
                    spdlog::error("Error: {}\n", NFD_GetError());
                }
            }
            if (ImGui::MenuItem("Open Recent")) {
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
        ImGui::EndMenuBar();
    }
}

void Ui::ShowCpuState() {
    if (emulator_context != nullptr) {
        auto cpu_state = debugger.GetCpuState();
        if (ImGui::BeginTable("cpu_registers", 2,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
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
    } else {
        ImGui::Text("Load a ROM to view NES CPU state");
    }
}

void Ui::ShowPpuState() {}

void Ui::ShowPpuMemory() {
    if (emulator_context == nullptr) {
        ImGui::Text("Load a ROM to view its memory");
        return;
    }

    static std::vector<byte> ppu_memory;
    ppu_memory.reserve(0x4000);
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
    static int pallette_id = 0;
    if (ImGui::InputInt("Palette ID", &pallette_id)) {
        if (pallette_id < 0) {
            pallette_id = 0;
        }

        if (pallette_id > 7) {
            pallette_id = 7;
        }
    }

    auto left_pixels = RenderPixelsForPatternTable(pattern_table_state.left,
                                                   pattern_table_state.palettes, pallette_id);
    glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<unsigned char*>(left_pixels.data()));

    ImGui::Image((void*)(intptr_t)pattern_table_left_texture, ImVec2(385, 385));

    ImGui::Separator();

    auto right_pixels = RenderPixelsForPatternTable(pattern_table_state.right,
                                                    pattern_table_state.palettes, pallette_id);
    glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<unsigned char*>(right_pixels.data()));
    ImGui::Image((void*)(intptr_t)pattern_table_right_texture, ImVec2(385, 385));
}

std::vector<Pixel> Ui::RenderPixelsForPatternTable(std::span<byte, 4096> pattern_table,
                                                   std::span<byte, 32> nes_palette,
                                                   int palette_id) const {
    std::vector<Pixel> pixels(128 * 128);

    for (size_t column = 0; column < 128; column++) {
        size_t tile_column = column / 8;
        size_t pixel_column_in_tile = column % 8;

        for (size_t row = 0; row < 128; row++) {
            size_t tile_row = row / 8;
            size_t pixel_row_in_tile = 7 - (row % 8);

            // TODO: Find out why this works correctly with pixel_{column,row}_in_tile
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
            auto nes_palette_color_index = (palette_id * 4) + color_index;

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
