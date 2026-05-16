#include "gameboy.hpp"

namespace gbe {

GameBoy::GameBoy() = default;
GameBoy::~GameBoy() = default;

void GameBoy::step() {
    ++cycles_;
}

}  // namespace gbe
