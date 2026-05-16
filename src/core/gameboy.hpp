#pragma once

#include <cstdint>

namespace gbe {

// Placeholder. The real GameBoy class will own all subsystems.
class GameBoy {
public:
    GameBoy();
    ~GameBoy();

    // Stepping API will live here.
    void step();

private:
    std::uint64_t cycles_ = 0;
};

}  // namespace gbe
