#include <gtest/gtest.h>

#include "core/gameboy.hpp"

TEST(Sanity, ConstructAndStep) {
    gbe::GameBoy gb;
    gb.step();
    SUCCEED();
}
