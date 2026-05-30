#pragma once

#include <array>
#include <cstdint>
#include <span>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace gbe {

// Owns the SDL3 window, renderer, and streaming texture that present
// the Game Boy framebuffer. The frontend feeds it palette-indexed
// pixel buffers via present(); Window converts them to ARGB at upload
// time, keeping the palette as a frontend concern per architecture.md
// "rendering" — the core stores indices, not colors.
class Window {
public:
    static constexpr int gb_width = 160;
    static constexpr int gb_height = 144;
    static constexpr int initial_scale = 4;
    static constexpr std::size_t pixel_count =
        static_cast<std::size_t>(gb_width) * static_cast<std::size_t>(gb_height);

    Window();
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // Creates the window, renderer, and streaming texture. Logs and
    // returns false on the first SDL call that fails; the destructor
    // releases whatever was created up to that point.
    [[nodiscard]] bool init();

    // Uploads a 160x144 palette-indexed framebuffer (values 0-3) and
    // presents the frame. Caller's span must be sized pixel_count.
    void present(std::span<const std::uint8_t> framebuffer);

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;

    // Palette-index -> ARGB scratch buffer. Member rather than local
    // to avoid 90 KiB of stack churn per present() call.
    std::array<std::uint32_t, pixel_count> argb_scratch_{};
};

}  // namespace gbe
