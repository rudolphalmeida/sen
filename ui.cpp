#include <cstddef>
#include <memory>
#include <optional>

#include <GLFW/glfw3.h>
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
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
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

        {
            if (show_cpu_registers && emulator_context != nullptr) {
                if (ImGui::Begin("CPU Registers", &show_cpu_registers)) {
                    auto cpu_state = debugger.GetCpuState();

                    ImGui::Value("A", static_cast<unsigned int>(cpu_state.a));
                    ImGui::Value("X", static_cast<unsigned int>(cpu_state.x));
                    ImGui::Value("Y", static_cast<unsigned int>(cpu_state.y));
                    ImGui::Value("S", static_cast<unsigned int>(cpu_state.s));
                    ImGui::Value("PC", static_cast<unsigned int>(cpu_state.pc));
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
            if (show_vram) {
                if (ImGui::Begin("VRAM", &show_vram)) {
                }
                ImGui::End();
            }
        }

        {
            if (show_memory) {
                if (ImGui::Begin("Memory", &show_memory)) {
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

void Ui::StartEmulation() {
    emulation_running = true;
    emulator_context->Start();
}

void Ui::PauseEmulation() {}

void Ui::ResetEmulation() {}

void Ui::StopEmulation() {}
