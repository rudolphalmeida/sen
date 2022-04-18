//
// Created by rudolph on 18/4/22.
//

#include "ui.hxx"

int Ui::Run() {
    InitWindow("Sen - NES Emulator", SDL_WINDOWPOS_CENTERED,
               SDL_WINDOWPOS_CENTERED, 500, 500,
               SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    while (true) {
        cpu.Tick();
    }
}
void Ui::InitWindow(const char* title,
                    int x,
                    int y,
                    int width,
                    int height,
                    Uint32 flags) {
    window = SDL_CreateWindow(title, x, y, width, height, flags);
    if (!window) {
        spdlog::critical("Failed to create SDL2 window: {}", SDL_GetError());
        std::exit(-1);
    }

    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        spdlog::critical("Failed to initialize SDL2 renderer: {}",
                         SDL_GetError());
        SDL_Quit();
        std::exit(-1);
    }
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STATIC, width, height);
}
