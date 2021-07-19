#include "cmseg.h"

#include <mkb.h>
#include <log.h>
#include <patch.h>

#define ARRAY_LEN(x) (sizeof((x)) / sizeof((x)[0]))

namespace cmseg
{

enum class State
{
    Default,
    LoadMenu,
    EnterCm,
    SegActive,
    SegComplete,
};

static State s_state = State::Default;
static Seg s_seg_request;
static Chara s_chara_request;

static void (*s_g_reset_cm_course_tramp)();
static void (*s_create_final_stage_sprite_tramp)();

static mkb::CmEntry *s_overwritten_entry;
static mkb::CmEntryType s_overwritten_entry_type;

static void reset_screenfade_state()
{
    // Reset screenfade parameters to that of "begin fading back from black screen"
    mkb::g_screenfade_flags = 0x00000100;
    mkb::g_screenfade_color = 0x00000000;
    mkb::g_screenfading1 = 0x0000001a;
    mkb::g_screenfading2 = 0x0000001b;
}

//static void debug_print_course(mkb::CmEntry *course, u32 entry_count)
//{
//    static const char *type_strs[] = {"CMET_IF", "CMET_THEN", "CMET_INFO", "CMET_END"};
//
//    mkb::OSReport("Course entry count: %d\n", entry_count);
//    for (u32 i = 0; i < entry_count; i++)
//    {
//        mkb::CmEntry &entry = course[i];
//        mkb::OSReport("%s: n = %d, v = %d\n", type_strs[entry.type], entry.arg, entry.value);
//    }
//    mkb::OSReport("\n");
//}

/**
 * Create a new course in an existing one by inserting a CMET_END entry
 */
static void gen_course(mkb::CmEntry *course, u32 start_course_stage_num, u32 stage_count)
{
    s32 start_entry_idx = -1;
    s32 end_entry_idx = -1;

    u32 curr_stage_count = 0;
    for (s32 i = 0;; i++)
    {
        if (course[i].type == mkb::CMET_INFO && course[i].arg == 0)
        {
            curr_stage_count++;
            if (curr_stage_count == start_course_stage_num)
            {
                start_entry_idx = i;
            }
            else if (curr_stage_count == start_course_stage_num + stage_count)
            {
                end_entry_idx = i;
                break;
            }
        }
        else if (course[i].type == mkb::CMET_END)
        {
            if (curr_stage_count == start_course_stage_num + stage_count - 1)
            {
                end_entry_idx = i; // This CmEntry is one past the end - we tack on a CMET_END entry ourselves
            }
            break;
        }
    }

    // Check if we found stage indices
    MOD_ASSERT(start_entry_idx > -1);
    MOD_ASSERT(end_entry_idx > -1);

    // Write new course end marker
    s_overwritten_entry = &course[end_entry_idx];
    s_overwritten_entry_type = course[end_entry_idx].type;
    course[end_entry_idx].type = mkb::CMET_END;

    s16 first_stage_id = static_cast<s16>(course[start_entry_idx].value);
    mkb::mode_info.cm_course_stage_num = start_course_stage_num;
    mkb::mode_info.cm_stage_id = first_stage_id; // Record first stage in course
    mkb::current_cm_entry = &course[start_entry_idx + 1];
    mkb::g_some_cm_stage_id2 = first_stage_id;

    // Make up "previous" stage for "current" stage
    mkb::CmStage &curr_stage = mkb::cm_player_progress[0].curr_stage;
    curr_stage.stage_course_num = start_course_stage_num - 1;
    curr_stage.stage_id = first_stage_id - 1;

    // Next stage for player is the first one we want to start on
    mkb::CmStage &next_stage = mkb::cm_player_progress[0].next_stages[0];
    next_stage.stage_course_num = start_course_stage_num;
    next_stage.stage_id = first_stage_id;
}

static void state_load_menu()
{
    mkb::g_some_other_flags &= ~mkb::OF_GAME_PAUSED; // Unpause the game to avoid weird darkening issues
    mkb::main_mode_request = mkb::MD_SEL;
    // Using REINIT instead of INIT seems to prevent weird game state issues, like
    // the Final Stage sprite being shown when loading a stage in story mode
    mkb::sub_mode_request = mkb::SMD_SEL_NGC_REINIT;

    // Set menu state to have chosen Main Game -> Challenge Mode
    mkb::menu_stack_ptr = 1;
    mkb::g_menu_stack[0] = 0; // ??
    mkb::g_menu_stack[1] = 7; // Main game
    mkb::g_focused_root_menu = 0;
    mkb::g_focused_maingame_menu = 1;

    reset_screenfade_state();

    s_state = State::EnterCm;
}

static const mkb::ApeCharacter s_ape_charas[] = {
    mkb::APE_AIAI,
    mkb::APE_MEEMEE,
    mkb::APE_BABY,
    mkb::APE_GONGON,
    mkb::APE_MADH,
    mkb::APE_WHALE,
};

static void state_enter_cm()
{

    // TODO enforce 1-player game
    // TODO character, lives

    mkb::num_players = 1;
    mkb::ApeCharacter real_chara;
    if (s_chara_request == Chara::RandomMainFour)
    {
        real_chara = s_ape_charas[mkb::rand() % 4];
    }
    else if (s_chara_request == Chara::RandomAll)
    {
        real_chara = s_ape_charas[mkb::rand() % 6];
    }
    else
    {
        real_chara = s_ape_charas[static_cast<u32>(s_chara_request)];
    }
    mkb::active_monkey_id = real_chara;

    mkb::enter_challenge_mode();

    // TODO restore main menu state to look like we entered Challenge Mode
    // TODO do this before loading REINIT to avoid mode.cnt = 0 error (and in Go To Story Mode too)

    reset_screenfade_state();

    s_state = State::SegActive;
}

static void state_seg_active()
{
    if (mkb::main_mode != mkb::MD_GAME)
    {
        s_overwritten_entry->type = s_overwritten_entry_type; // Restore original challenge mode course
        s_state = State::Default;
    }
}

static void state_seg_complete()
{
    if (mkb::main_mode != mkb::MD_GAME)
    {
        s_overwritten_entry->type = s_overwritten_entry_type; // Restore original challenge mode course
        s_state = State::Default;
    }
}

void init_seg()
{
    mkb::CmEntry *course = nullptr;
    u32 start_course_stage_num = 0;
    mkb::mode_flags &= ~(
        mkb::MF_G_PLAYING_MASTER_COURSE
        | mkb::MF_PLAYING_EXTRA_COURSE
        | mkb::MF_PLAYING_MASTER_NOEX_COURSE
        | mkb::MF_PLAYING_MASTER_EX_COURSE);
    switch (s_seg_request)
    {
        case Seg::Beginner1:
        {
            mkb::curr_difficulty = mkb::DIFF_BEGINNER;
            course = mkb::beginner_noex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::BeginnerExtra:
        {
            mkb::curr_difficulty = mkb::DIFF_BEGINNER;
            mkb::mode_flags |= mkb::MF_PLAYING_EXTRA_COURSE;
            course = mkb::beginner_ex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::Advanced1:
        {
            mkb::curr_difficulty = mkb::DIFF_ADVANCED;
            course = mkb::advanced_noex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::Advanced11:
        {
            mkb::curr_difficulty = mkb::DIFF_ADVANCED;
            course = mkb::advanced_noex_cm_entries;
            start_course_stage_num = 11;
            break;
        }
        case Seg::Advanced21:
        {
            mkb::curr_difficulty = mkb::DIFF_ADVANCED;
            course = mkb::advanced_noex_cm_entries;
            start_course_stage_num = 21;
            break;
        }
        case Seg::AdvancedExtra:
        {
            mkb::curr_difficulty = mkb::DIFF_ADVANCED;
            mkb::mode_flags |= mkb::MF_PLAYING_EXTRA_COURSE;
            course = mkb::advanced_ex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::Expert1:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            course = mkb::expert_noex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::Expert11:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            course = mkb::expert_noex_cm_entries;
            start_course_stage_num = 11;
            break;
        }
        case Seg::Expert21:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            course = mkb::expert_noex_cm_entries;
            start_course_stage_num = 21;
            break;
        }
        case Seg::Expert31:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            course = mkb::expert_noex_cm_entries;
            start_course_stage_num = 31;
            break;
        }
        case Seg::Expert41:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            course = mkb::expert_noex_cm_entries;
            start_course_stage_num = 41;
            break;
        }
        case Seg::ExpertExtra:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            mkb::mode_flags |= mkb::MF_PLAYING_EXTRA_COURSE;
            course = mkb::expert_ex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::Master1:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            mkb::mode_flags |= mkb::MF_PLAYING_EXTRA_COURSE
                               | mkb::MF_G_PLAYING_MASTER_COURSE
                               | mkb::MF_PLAYING_MASTER_NOEX_COURSE;
            course = mkb::master_noex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
        case Seg::MasterExtra:
        {
            mkb::curr_difficulty = mkb::DIFF_EXPERT;
            // Magic set of flags used in Master Extra,
            // can't be bothered to reverse all of them
            mkb::mode_flags = 0x0280071D;
            course = mkb::master_ex_cm_entries;
            start_course_stage_num = 1;
            break;
        }
    }
    gen_course(course, start_course_stage_num, 2);
}

void request_cm_seg(Seg seg, Chara chara)
{
    s_seg_request = seg;
    s_chara_request = chara;
    if (s_state == State::SegActive || s_state == State::SegComplete)
    {
        s_overwritten_entry->type = s_overwritten_entry_type; // Restore original challenge mode course
    }
    if (mkb::main_mode == mkb::MD_SEL) s_state = State::EnterCm; // Load challenge mode directly
    else s_state = State::LoadMenu; // Load main menu first
}

void init()
{
    s_g_reset_cm_course_tramp = patch::hook_function(
        mkb::g_reset_cm_course, []()
        {
            s_g_reset_cm_course_tramp();
            if (s_state == State::SegActive) init_seg();
        }
    );

    // Hide "Final Stage" sprite
    s_create_final_stage_sprite_tramp = patch::hook_function(
        mkb::create_final_stage_sprite, []()
        {
            if (s_state != State::SegActive || s_overwritten_entry_type == mkb::CMET_END)
            {
                s_create_final_stage_sprite_tramp();
            }
        }
    );
}

void tick()
{
    if (s_state == State::LoadMenu) state_load_menu();
    else if (s_state == State::EnterCm) state_enter_cm();
    else if (s_state == State::SegActive) state_seg_active();
    else if (s_state == State::SegComplete) state_seg_complete();
}

}