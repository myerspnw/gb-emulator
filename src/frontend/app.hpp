#pragma once

#include "window.hpp"

namespace gbe {

// Top-level frontend orchestrator. Owns the SDL window and (in later
// phases) the audio device, input mapper, and the GameBoy core. Run
// via the static entry point; SDL_Init/SDL_Quit bracket App's lifetime
// so any SDL-using member is destroyed before SDL_Quit.
class App {
public:
    static int run();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

private:
    App() = default;
    ~App() = default;

    [[nodiscard]] bool init();
    int main_loop();

    Window window_;
};

}  // namespace gbe
