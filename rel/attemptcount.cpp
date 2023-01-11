#include "attemptcount.h"
#include <timerdisp.h>
#include "draw.h"
#include "pad.h"

#include <mkb.h>

#include <cstring>
#include "mkb2_ghidra.h"
#include "pref.h"

namespace attemptcount {

static u32 attempts = 0;
static constexpr s32 X = 380;
static constexpr s32 Y = 24 + 16*3;

static void show_counter() {
    draw::debug_text(X, Y, draw::LIGHT_GREEN, "ATTEMPTS:");
    draw::debug_text(X + 115, Y, draw::LIGHT_GREEN, "%d", attempts);
}

static void track_attempts() {
    bool paused_now = *reinterpret_cast<u32*>(0x805BC474) & 8;
    if ((mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 1)
    && !paused_now) {
        attempts = attempts + 1;
    }
}

void tick() {
    if (pref::get_attempt_count()) {
        track_attempts();
        if (pad::button_pressed(mkb::PAD_BUTTON_B)) {
            attempts = attempts + 1;
        }
        if (pad::button_pressed(mkb::PAD_BUTTON_Y)) {
            attempts = attempts + 50;
        }
    }
    else attempts = 0;
}

void disp() {
    if (pref::get_attempt_count()) {
        //show_counter();
    }
}
} // Namespace attemptcount