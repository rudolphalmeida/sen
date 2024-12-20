//
// Created by rudolph on 19/12/24.
//

#include "ui.hxx"

PostProcessedData NoFilter::PostProcess(const std::span<unsigned short, 61440> nes_pixels, int) {
    if (pixels.size() != NES_WIDTH * NES_HEIGHT) {
        pixels.resize(NES_WIDTH * NES_HEIGHT);
    }

    for (int y = 0; y < NES_HEIGHT; y++) {
        for (int x = 0; x < NES_WIDTH; x++) {
            const byte color_index = nes_pixels[y * NES_WIDTH + x];
            pixels[y * NES_WIDTH + x] = PALETTE_COLORS[color_index];
        }
    }

    return { .data = pixels.data(), .width = NES_WIDTH, .height = NES_HEIGHT};
}

PostProcessedData NtscFilter::PostProcess(const std::span<unsigned short, 61440> nes_pixels,
                                          int current_scale_factor) {
    if (current_scale_factor != scale_factor) {
        pixels.resize(NES_WIDTH * NES_HEIGHT * current_scale_factor * current_scale_factor);
        std::ranges::for_each(pixels, [](auto& pixel) {
            pixel = Pixel();
        });
        crt_resize(&crt, NES_WIDTH * current_scale_factor, NES_HEIGHT * current_scale_factor, CRT_PIX_FORMAT_RGB, reinterpret_cast<unsigned char*>(pixels.data()));
    }

    ntsc.data = nes_pixels.data();
    ntsc.w = NES_WIDTH;
    ntsc.h = NES_HEIGHT;
    ntsc.hue = hue;
    ntsc.dot_crawl_offset = 1;
    ntsc.border_color = 255;
    ntsc.xoffset = 0;
    ntsc.yoffset = 0;
    crt_modulate(&crt, &ntsc);
    crt_demodulate(&crt, noise);

    return { .data = pixels.data(), .width = NES_WIDTH * scale_factor, .height = NES_HEIGHT * scale_factor};
}

