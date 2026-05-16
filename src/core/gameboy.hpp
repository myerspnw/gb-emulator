#pragma once

#include <cstdint>

namespace gbe {

// Placeholder. The real GameBoy class will own all subsystems.
class GameBoy {
public:
    GameBoy();
    ~GameBoy() = default;

    GameBoy(const GameBoy&) = delete;
    GameBoy& operator=(const GameBoy&) = delete;
    GameBoy(GameBoy&&) = default;
    GameBoy& operator=(GameBoy&&) = default;

    void step();

private:
    std::uint64_t cycles_ = 0;
};

}  // namespace gbe
