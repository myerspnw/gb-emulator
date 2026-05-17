#pragma once

#include <cstdint>

namespace gbe {

// Placeholder. The real GameBoy class will own all subsystems.
//
// Non-copyable and non-movable: subsystems will hold back-references
// to the Bus, so the GameBoy and its subobjects must have stable
// addresses for the lifetime of the instance. Wrap in
// std::unique_ptr<GameBoy> if the frontend needs to relocate one.
class GameBoy {
public:
    GameBoy();
    ~GameBoy() = default;

    GameBoy(const GameBoy&) = delete;
    GameBoy& operator=(const GameBoy&) = delete;
    GameBoy(GameBoy&&) = delete;
    GameBoy& operator=(GameBoy&&) = delete;

    void step();

private:
    std::uint64_t cycles_ = 0;
};

}  // namespace gbe
