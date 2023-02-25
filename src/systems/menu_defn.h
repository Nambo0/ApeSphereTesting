#pragma once

#include "mkb/mkb.h"

#include "pref.h"

namespace menu_defn {

enum class WidgetType {
    Text,
    ColoredText,
    Header,
    Checkbox,
    GetSetCheckbox,
    Separator,
    Menu,
    FloatView,
    Choose,
    Button,
    Custom,
};

struct TextWidget {
    const char* label;            // For static text
    const char* (*label_func)();  // For dynamic text
};

struct ColoredTextWidget {
    const char* label;
    mkb::GXColor color;
};

// Just a different color TextWidget
struct HeaderWidget {
    const char* label;
};

struct CheckboxWidget {
    const char* label;
    pref::BoolPref pref;
};

// For the rare cases a checkbox doesn't correspond to a preference
struct GetSetCheckboxWidget {
    const char *label;
    bool (*get)();
    void (*set)(bool);
};

struct MenuWidget {
    const char* label;
    struct Widget* widgets;
    u32 num_widgets;
    // It's too convenient to store currently selected menu entry in the widget itself,
    // even if it violates the otherwise immutable nature of the menu definition
    u32 selected_idx;
};

struct FloatViewWidget {
    const char* label;
    f32 (*get)();
};

struct ChooseWidget {
    const char* label;
    const char** choices;
    u16 num_choices;
    pref::U8Pref pref;
};

namespace ButtonFlags {
enum {
    CloseMenu = 1 << 0,  // Close menu after pushed
    GoBack = 1 << 1,     // Go back to parent menu after pushed
};
}

struct ButtonWidget {
    const char* label;
    void (*push)();  // Runs when pushed. Can be null
    u32 flags;
};

struct CustomWidget {
    void (*draw)();
};

struct Widget {
    WidgetType type;
    union {
        TextWidget text;
        ColoredTextWidget colored_text;
        HeaderWidget header;
        CheckboxWidget checkbox;
        GetSetCheckboxWidget get_set_checkbox;
        MenuWidget menu;
        FloatViewWidget float_view;
        ChooseWidget choose;
        ButtonWidget button;
        CustomWidget custom;
    };
};

extern MenuWidget root_menu;

void init();

}  // namespace menu_defn
