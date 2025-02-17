#pragma once

#include <span>
#include <vector>

#define CRT_SYSTEM CRT_SYSTEM_NES
#include "constants.hxx"
#include "crt_core.h"

struct Pixel {
    byte r{};
    byte g{};
    byte b{};
};

static constexpr Pixel PALETTE_COLORS[0x40] = {
    Pixel{84, 84, 84},    Pixel{0, 30, 116},    Pixel{8, 16, 144},    Pixel{48, 0, 136},
    Pixel{68, 0, 100},    Pixel{92, 0, 48},     Pixel{84, 4, 0},      Pixel{60, 24, 0},
    Pixel{32, 42, 0},     Pixel{8, 58, 0},      Pixel{0, 64, 0},      Pixel{0, 60, 0},
    Pixel{0, 50, 60},     Pixel{0, 0, 0},       Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{152, 150, 152}, Pixel{8, 76, 196},    Pixel{48, 50, 236},   Pixel{92, 30, 228},
    Pixel{136, 20, 176},  Pixel{160, 20, 100},  Pixel{152, 34, 32},   Pixel{120, 60, 0},
    Pixel{84, 90, 0},     Pixel{40, 114, 0},    Pixel{8, 124, 0},     Pixel{0, 118, 40},
    Pixel{0, 102, 120},   Pixel{0, 0, 0},       Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{236, 238, 236}, Pixel{76, 154, 236},  Pixel{120, 124, 236}, Pixel{176, 98, 236},
    Pixel{228, 84, 236},  Pixel{236, 88, 180},  Pixel{236, 106, 100}, Pixel{212, 136, 32},
    Pixel{160, 170, 0},   Pixel{116, 196, 0},   Pixel{76, 208, 32},   Pixel{56, 204, 108},
    Pixel{56, 180, 204},  Pixel{60, 60, 60},    Pixel{0, 0, 0},       Pixel{0, 0, 0},
    Pixel{236, 238, 236}, Pixel{168, 204, 236}, Pixel{188, 188, 236}, Pixel{212, 178, 236},
    Pixel{236, 174, 236}, Pixel{236, 174, 212}, Pixel{236, 180, 176}, Pixel{228, 196, 144},
    Pixel{204, 210, 120}, Pixel{180, 222, 120}, Pixel{168, 226, 144}, Pixel{152, 226, 180},
    Pixel{160, 214, 228}, Pixel{160, 162, 160}, Pixel{0, 0, 0},       Pixel{0, 0, 0},
};

struct PostProcessedData {
    Pixel* data{};
    int width{};
    int height{};
};

class Filter {
  public:
    virtual ~Filter() = default;
    virtual PostProcessedData
    PostProcess(std::span<unsigned short, 61440> nes_pixels, int scale_factor) = 0;
};

class NoFilter final: public Filter {
  public:
    NoFilter() : pixels(NES_WIDTH * NES_HEIGHT) {}

    PostProcessedData
    PostProcess(std::span<unsigned short, 61440> nes_pixels, int scale_factor) override;

  private:
    std::vector<Pixel> pixels{};
};

class NtscFilter final: public Filter {
  public:
    explicit NtscFilter(const int initial_scale_factor) :
        pixels(NES_WIDTH * NES_HEIGHT * initial_scale_factor * initial_scale_factor),
        scale_factor{initial_scale_factor} {
        crt_init(
            &crt,
            NES_WIDTH * initial_scale_factor,
            NES_HEIGHT * initial_scale_factor,
            CRT_PIX_FORMAT_RGB,
            reinterpret_cast<unsigned char*>(pixels.data())
        );
        crt.blend = 1;
        crt.scanlines = 0;
    }

    PostProcessedData
    PostProcess(std::span<unsigned short, 61440> nes_pixels, int scale_factor) override;

  private:
    std::vector<Pixel> pixels{};
    int scale_factor{};

    CRT crt{};
    NTSC_SETTINGS ntsc{};
    int noise = 2;
    int hue = 350;
};
