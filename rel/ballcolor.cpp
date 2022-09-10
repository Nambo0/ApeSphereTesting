#include "ballcolor.h"

#include <mkb.h>

#include <cstring>
#include "draw.h"
#include "heap.h"
#include "memstore.h"
#include "mkb2_ghidra.h"
#include "pad.h"
#include "patch.h"
#include "pref.h"
#include "timer.h"

namespace ballcolor {

static u8 convert_to_ballColorID(u8 color_choice){
    if(color_choice == 0) return 3;
    else return color_choice - 1;
}

static u8 convert_to_apeColorID(u8 color_choice){
    if(color_choice == 0) return 0;
    else return color_choice - 1;
}

void tick() {
    // Set ball & ape color to color ID
    if(mkb::balls[mkb::curr_player_idx].ape != nullptr){ // Check for nullpointers first
        mkb::balls[mkb::curr_player_idx].ape->color_index = convert_to_apeColorID(pref::get_ape_color());
        mkb::balls[mkb::curr_player_idx].g_ball_color_index = convert_to_ballColorID(pref::get_ball_color());
    }
}

} // namespace ballcolor
