#include "app.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>

namespace gbe {

int App::run() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return 1;
    }
    spdlog::info("SDL initialised (version {}.{}.{})",
                 SDL_VERSIONNUM_MAJOR(SDL_VERSION),
                 SDL_VERSIONNUM_MINOR(SDL_VERSION),
                 SDL_VERSIONNUM_MICRO(SDL_VERSION));

    int rc = 1;
    {
        App app;
        if (app.init()) {
            rc = app.main_loop();
        }
    }  // App (and its Window) destroyed before SDL_Quit.

    SDL_Quit();
    return rc;
}

bool App::init() {
    return window_.init();
}

int App::main_loop() {
    // Phase 0 placeholder: four vertical bands of the four DMG palette
    // indices, proving the texture-upload + integer-scale pipeline
    // works end-to-end. Replaced by gameboy.framebuffer() once the PPU
    // lands.
    std::array<std::uint8_t, Window::pixel_count> framebuffer{};
    constexpr auto width = static_cast<std::size_t>(Window::gb_width);
    constexpr auto height = static_cast<std::size_t>(Window::gb_height);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            framebuffer[(y * width) + x] = static_cast<std::uint8_t>((x * 4U) / width);
        }
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        }

        window_.present(framebuffer);
    }

    spdlog::info("Window closed; exiting.");
    return 0;
}

}  // namespace gbe
