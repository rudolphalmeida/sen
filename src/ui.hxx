//
// Created by rudolph on 18/4/22.
//

#ifndef SEN_SRC_UI_HXX_
#define SEN_SRC_UI_HXX_

#include <memory>
#include <utility>

#include <SDL2/SDL.h>

#include "cpu.h"
#include "mmu.h"
#include "options.hxx"

class Ui {
   public:
    explicit Ui(Options&& options, Cpu&& cpu, std::shared_ptr<Mmu> mmu)
        : options{std::move(options)},
          cpu{std::move(cpu)},
          mmu{std::move(mmu)} {}

    int Run();

    ~Ui();

   private:
    Options options;

    // NES components
    Cpu cpu;
    std::shared_ptr<Mmu> mmu;

    // SDL2 window and controllers
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};

    SDL_Event event{};
    bool quit{false};

    void InitWindow(const char* title,
                    int x,
                    int y,
                    int width,
                    int height,
                    Uint32 flags);
    void PollEvents();
};

#endif  // SEN_SRC_UI_HXX_
