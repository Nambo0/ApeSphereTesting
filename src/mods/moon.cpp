#include "moon.h"
#include "mkb/mkb.h"
#include "systems/pref.h"
#include "utils/patch.h"

namespace moon {

static void moon_gravity() {
    bool paused_now = *reinterpret_cast<u32*>(0x805BC474) & 8;  // Makes paused_now work
    if (mkb::mode_info.stage_time_frames_remaining < mkb::mode_info.stage_time_limit &&
        !paused_now) {
        mkb::balls[mkb::curr_player_idx].vel.y += .005;
    }
}

void tick() {
    if (pref::get_moon()) {
        //moon_gravity();
        for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
            if (mkb::item_pool_info.status_list[i] == 0) continue;                                  // skip if its inactive
            mkb::Item& item = mkb::items[i];                                                        // shorthand: current item in the list = "item"
            if (item.coin_type != 1) continue;                                                      // skip if its not a bunch
            if (mkb::stagedef->coli_header_list[item.itemgroup_idx].anim_group_id != 101) continue; // skip if anything but 101 anim ID
            item.scale = 5;
        }
    }
}

static patch::Tramp<decltype(&mkb::item_coin_disp)> s_item_coin_disp_tramp;

void new_effect_draw(mkb::Effect* effect){
    
    mkb::spawn_effect(effect);
}

void init() {
    patch::hook_function(s_item_coin_disp_tramp, mkb::item_coin_disp, [](mkb::Item* item){
        if (item->coin_type == 1 && mkb::stagedef->coli_header_list[item->itemgroup_idx].anim_group_id == 101){
            item->scale = 2;
        }
        s_item_coin_disp_tramp.dest(item);
    });
    patch::write_branch_bl(reinterpret_cast<void*>(0x80316c9c), )
}

}  // Namespace moon