#include "app.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

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
    SDL_Quit();
    return 0;
}

}  // namespace gbe
