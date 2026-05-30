#include "window.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

namespace gbe {

namespace {

// Canonical DMG greenscale palette in ARGB8888. Index 0 = lightest,
// 3 = darkest, matching DMG palette-index semantics. Per architecture.md
// the palette lives in the frontend; v2 may expose user-selectable
// palettes without touching the core.
constexpr std::array<std::uint32_t, 4> dmg_palette = {
    0xFFE0F8D0U,
    0xFF88C070U,
    0xFF346856U,
    0xFF081820U,
};

constexpr int pitch_bytes = Window::gb_width * static_cast<int>(sizeof(std::uint32_t));

}  // namespace

Window::Window() = default;

Window::~Window() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
    }
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

bool Window::init() {
    window_ = SDL_CreateWindow("Game Boy Emulator",
                               gb_width * initial_scale,
                               gb_height * initial_scale,
                               SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr) {
        spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
        return false;
    }

    // VSync is best-effort: a software renderer may not support it.
    // Without VSync the loop still renders correctly, but frame pacing
    // falls back to "as fast as events drain" until the PPU paces us.
    if (!SDL_SetRenderVSync(renderer_, 1)) {
        spdlog::warn("SDL_SetRenderVSync failed (continuing without vsync): {}", SDL_GetError());
    }

    // Renderer scales 160x144 logical pixels to the window with integer
    // scaling + letterboxing/pillarboxing, preserving the DMG aspect
    // ratio at any window size.
    if (!SDL_SetRenderLogicalPresentation(
            renderer_, gb_width, gb_height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        spdlog::error("SDL_SetRenderLogicalPresentation failed: {}", SDL_GetError());
        return false;
    }

    texture_ = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, gb_width, gb_height);
    if (texture_ == nullptr) {
        spdlog::error("SDL_CreateTexture failed: {}", SDL_GetError());
        return false;
    }

    // Nearest-neighbour keeps pixel edges crisp under integer upscale.
    if (!SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST)) {
        spdlog::warn("SDL_SetTextureScaleMode failed: {}", SDL_GetError());
    }

    spdlog::info("Window opened: {}x{} (logical {}x{}, integer-scaled)",
                 gb_width * initial_scale,
                 gb_height * initial_scale,
                 gb_width,
                 gb_height);
    return true;
}

void Window::present(std::span<const std::uint8_t> framebuffer) {
    assert(framebuffer.size() == pixel_count);

    for (std::size_t i = 0; i < pixel_count; ++i) {
        const auto idx = static_cast<std::size_t>(framebuffer[i]);
        assert(idx < dmg_palette.size());
        argb_scratch_[i] = dmg_palette[idx];
    }

    SDL_UpdateTexture(texture_, nullptr, argb_scratch_.data(), pitch_bytes);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

}  // namespace gbe
