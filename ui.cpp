#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

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

#if defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(NES_WIDTH * scale_factor, NES_HEIGHT * scale_factor,
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

        ShowMenuBar();
        ImGui::ShowDemoWindow();

        {
            if (show_cpu_registers && emulator_context != nullptr) {
                if (ImGui::Begin("CPU Registers", &show_cpu_registers)) {
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
                                ImGui::Text("0x%.4X: %s 0x%.2X 0x%.2X", executed_opcode.pc,
                                            opcode.label, executed_opcode.arg1,
                                            executed_opcode.arg2);
                                break;
                        }
                    }
                }
                ImGui::End();
            }
        }

        {
            if (show_ppu_registers) {
                if (ImGui::Begin("PPU Registers", &show_ppu_registers)) {
                }
                ImGui::End();
            }
        }

        {
            if (show_pattern_tables) {
                if (ImGui::Begin("Pattern Tables", &show_pattern_tables)) {
                    if (ImGui::BeginTabBar("pattern_tables")) {
                        auto pattern_table_state = debugger.GetPatternTableState();
                        if (ImGui::BeginTabItem("Pattern Table 0")) {
                            auto left_pixels =
                                RenderPixelsForPatternTable(pattern_table_state.left);
                            glBindTexture(GL_TEXTURE_2D, pattern_table_left_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB,
                                         GL_UNSIGNED_BYTE,
                                         reinterpret_cast<unsigned char*>(left_pixels.data()));

                            ImGui::Image((void*)(intptr_t)pattern_table_left_texture,
                                         ImVec2(512, 512));
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Pattern Table 1")) {
                            auto right_pixels =
                                RenderPixelsForPatternTable(pattern_table_state.right);
                            glBindTexture(GL_TEXTURE_2D, pattern_table_right_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB,
                                         GL_UNSIGNED_BYTE,
                                         reinterpret_cast<unsigned char*>(right_pixels.data()));
                            ImGui::Image((void*)(intptr_t)pattern_table_right_texture,
                                         ImVec2(512, 512));
                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }
                }
                ImGui::End();
            }
        }

        {
            if (show_cpu_memory) {
                if (ImGui::Begin("CPU Memory", &show_cpu_memory)) {
                }
                ImGui::End();
            }
        }

        {
            if (show_ppu_memory) {
                if (ImGui::Begin("PPU Memory", &show_ppu_memory)) {
                    static std::vector<byte> ppu_memory;
                    ppu_memory.reserve(0x4000);
                    debugger.LoadPpuMemory(ppu_memory);
                    static MemoryEditor ppu_mem_edit;
                    ppu_mem_edit.ReadOnly = true;
                    ppu_mem_edit.DrawContents(ppu_memory.data(), 0x4000);
                }
                ImGui::End();
            }
        }

        {
            if (show_cart_info) {
                if (ImGui::Begin("Cartridge Info", &show_cart_info)) {
                }
                ImGui::End();
            }
        }

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
    if (ImGui::BeginMainMenuBar()) {
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

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Scale")) {
                for (size_t i = 0; i < 5; i++) {
                    if (ImGui::MenuItem(SCALING_FACTORS[i], nullptr, scale_factor == i + 1)) {
                        scale_factor = static_cast<unsigned int>(i + 1);
                        spdlog::info("Changing window scale to {}", scale_factor);
                        glfwSetWindowSize(window, NES_WIDTH * scale_factor,
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
            if (ImGui::MenuItem("Pattern Tables", nullptr, show_pattern_tables)) {
                show_pattern_tables = !show_pattern_tables;
            }
            if (ImGui::MenuItem("CPU Memory Viewer", nullptr, show_cpu_memory)) {
                show_cpu_memory = !show_cpu_memory;
            }
            if (ImGui::MenuItem("PPU Memory Viewer", nullptr, show_ppu_memory)) {
                show_ppu_memory = !show_ppu_memory;
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

std::vector<Pixel> Ui::RenderPixelsForPatternTable(std::span<byte, 4096> pattern_table) const {
    std::vector<Pixel> pixels(128 * 128);

    for (size_t column = 0; column < 128; column++) {
        size_t tile_column = column / 8;
        size_t pixel_column_in_tile = column % 8;

        for (size_t row = 0; row < 128; row++) {
            size_t tile_row = row / 8;
            size_t pixel_row_in_tile = row % 8;

            size_t tile_index = tile_row + tile_column * 16;
            size_t pixel_row_bitplane_0_index = tile_index * 16 + pixel_row_in_tile;
            size_t pixel_row_bitplane_1_index = pixel_row_bitplane_0_index + 8;

            byte pixel_row_bitplane_0 = pattern_table[pixel_row_bitplane_0_index];
            byte pixel_row_bitplane_1 = pattern_table[pixel_row_bitplane_1_index];

            byte pixel_msb =
                (pixel_row_bitplane_1 & (1 << pixel_column_in_tile)) != 0 ? 0b10 : 0b00;
            byte pixel_lsb =
                (pixel_row_bitplane_0 & (1 << pixel_column_in_tile)) != 0 ? 0b01 : 0b00;

            byte palette_index = pixel_msb | pixel_lsb;
            size_t pixel_index = row + column * 128;

            switch (palette_index) {
                case 0b00:
                    pixels[pixel_index] = Pixel{92, 148, 252};
                    break;
                case 0b01:
                    pixels[pixel_index] = Pixel{128, 208, 16};
                    break;
                case 0b10:
                    pixels[pixel_index] = Pixel{0, 168, 0};
                    break;
                case 0b11:
                    pixels[pixel_index] = Pixel{0, 0, 0};
                    break;
            }
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
