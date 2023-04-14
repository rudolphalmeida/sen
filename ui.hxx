#pragma once

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_opengl.h>
#include <SDL_video.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>
#include <memory>

#include "constants.hxx"
#include "sen.hxx"
#include "util.hxx"

struct UiRenderingContext {
    SDL_Window* window{};
    SDL_GLContext gl_context{};
    ImGuiIO& io;

    UiRenderingContext(SDL_Window* window, SDL_GLContext gl_context, ImGuiIO& io)
        : window{window}, gl_context{gl_context}, io{io} {}

    ~UiRenderingContext() {
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

class Ui {
   private:
    std::unique_ptr<UiRenderingContext> rendering_context{};
    bool done{false};
    unsigned int scale_factor{3};

    void ShowMenuBar();

   public:
    Ui();

    void Run();

    ~Ui() {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
};
