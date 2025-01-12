#pragma once

#include <mkb.h>

namespace inputdisp {

enum class Color {
    Purple,
    Red,
    Orange,
    Yellow,
    Green,
    Blue,
    Pink,
    Black,
};

static constexpr u32 NUM_COLORS = 8;

void init();
void on_PADRead(mkb::PADStatus* statuses);
void tick();
void disp();

}  // namespace inputdisp
