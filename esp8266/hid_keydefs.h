/*
 * hid_keydefs.h
 *
 * Complete USB HID keyboard and consumer control definitions
 * Based on USB HID Usage Tables specification
 *
 * Supports multiple keyboard layouts for international users
 */

#ifndef HID_KEYDEFS_H
#define HID_KEYDEFS_H

#include <Arduino.h>

// ========== USB HID KEYBOARD USAGE IDs (Page 0x07) ==========

// Modifier bitmasks
#define MOD_LCTRL   0x01
#define MOD_LSHIFT  0x02
#define MOD_LALT    0x04
#define MOD_LGUI    0x08
#define MOD_RCTRL   0x10
#define MOD_RSHIFT  0x20
#define MOD_RALT    0x40
#define MOD_RGUI    0x80

// Keyboard layout country codes
enum KeyboardLayout {
    LAYOUT_US = 0,
    LAYOUT_UK = 6,
    LAYOUT_FRENCH = 8,
    LAYOUT_GERMAN = 9,
    LAYOUT_SPANISH = 10,
    LAYOUT_ITALIAN = 11,
    LAYOUT_JAPANESE = 15,
    LAYOUT_DVORAK = 99  // Non-standard, for US Dvorak
};

// Key name structure
struct KeyName {
    const char* normal;
    const char* shifted;  // nullptr if same as normal
};

// ========== MODIFIER NAMES ==========
const char* const PROGMEM MOD_NAMES[] = {
    "LCTRL",
    "LSHIFT",
    "LALT",
    "LGUI",
    "RCTRL",
    "RSHIFT",
    "RALT",
    "RGUI"
};

// ========== US QWERTY LAYOUT ==========
const KeyName PROGMEM USB_KEYS_US[256] = {
    {nullptr, nullptr},        // 0x00 - Reserved (no event)
    {nullptr, nullptr},        // 0x01 - ErrorRollOver
    {nullptr, nullptr},        // 0x02 - POSTFail
    {nullptr, nullptr},        // 0x03 - ErrorUndefined
    {"A", nullptr},            // 0x04
    {"B", nullptr},            // 0x05
    {"C", nullptr},            // 0x06
    {"D", nullptr},            // 0x07
    {"E", nullptr},            // 0x08
    {"F", nullptr},            // 0x09
    {"G", nullptr},            // 0x0A
    {"H", nullptr},            // 0x0B
    {"I", nullptr},            // 0x0C
    {"J", nullptr},            // 0x0D
    {"K", nullptr},            // 0x0E
    {"L", nullptr},            // 0x0F
    {"M", nullptr},            // 0x10
    {"N", nullptr},            // 0x11
    {"O", nullptr},            // 0x12
    {"P", nullptr},            // 0x13
    {"Q", nullptr},            // 0x14
    {"R", nullptr},            // 0x15
    {"S", nullptr},            // 0x16
    {"T", nullptr},            // 0x17
    {"U", nullptr},            // 0x18
    {"V", nullptr},            // 0x19
    {"W", nullptr},            // 0x1A
    {"X", nullptr},            // 0x1B
    {"Y", nullptr},            // 0x1C
    {"Z", nullptr},            // 0x1D
    {"1", "!"},                // 0x1E
    {"2", "@"},                // 0x1F
    {"3", "#"},                // 0x20
    {"4", "$"},                // 0x21
    {"5", "%"},                // 0x22
    {"6", "^"},                // 0x23
    {"7", "&"},                // 0x24
    {"8", "*"},                // 0x25
    {"9", "("},                // 0x26
    {"0", ")"},                // 0x27
    {"ENTER", nullptr},        // 0x28
    {"ESC", nullptr},          // 0x29
    {"BACKSPACE", nullptr},    // 0x2A
    {"TAB", nullptr},          // 0x2B
    {"SPACE", nullptr},        // 0x2C
    {"-", "_"},                // 0x2D
    {"=", "+"},                // 0x2E
    {"[", "{"},                // 0x2F
    {"]", "}"},                // 0x30
    {"\\", "|"},               // 0x31
    {"#", "~"},                // 0x32 - Non-US # and ~
    {";", ":"},                // 0x33
    {"'", "\""},               // 0x34
    {"`", "~"},                // 0x35
    {",", "<"},                // 0x36
    {".", ">"},                // 0x37
    {"/", "?"},                // 0x38
    {"CAPS_LOCK", nullptr},    // 0x39
    {"F1", nullptr},           // 0x3A
    {"F2", nullptr},           // 0x3B
    {"F3", nullptr},           // 0x3C
    {"F4", nullptr},           // 0x3D
    {"F5", nullptr},           // 0x3E
    {"F6", nullptr},           // 0x3F
    {"F7", nullptr},           // 0x40
    {"F8", nullptr},           // 0x41
    {"F9", nullptr},           // 0x42
    {"F10", nullptr},          // 0x43
    {"F11", nullptr},          // 0x44
    {"F12", nullptr},          // 0x45
    {"PRINT_SCREEN", nullptr}, // 0x46
    {"SCROLL_LOCK", nullptr},  // 0x47
    {"PAUSE", nullptr},        // 0x48
    {"INSERT", nullptr},       // 0x49
    {"HOME", nullptr},         // 0x4A
    {"PAGE_UP", nullptr},      // 0x4B
    {"DELETE", nullptr},       // 0x4C
    {"END", nullptr},          // 0x4D
    {"PAGE_DOWN", nullptr},    // 0x4E
    {"RIGHT_ARROW", nullptr},  // 0x4F
    {"LEFT_ARROW", nullptr},   // 0x50
    {"DOWN_ARROW", nullptr},   // 0x51
    {"UP_ARROW", nullptr},     // 0x52
    {"NUM_LOCK", nullptr},     // 0x53
    {"KP_/", nullptr},         // 0x54 - Keypad /
    {"KP_*", nullptr},         // 0x55
    {"KP_-", nullptr},         // 0x56
    {"KP_+", nullptr},         // 0x57
    {"KP_ENTER", nullptr},     // 0x58
    {"KP_1", "END"},           // 0x59
    {"KP_2", "DOWN"},          // 0x5A
    {"KP_3", "PGDN"},          // 0x5B
    {"KP_4", "LEFT"},          // 0x5C
    {"KP_5", nullptr},         // 0x5D
    {"KP_6", "RIGHT"},         // 0x5E
    {"KP_7", "HOME"},          // 0x5F
    {"KP_8", "UP"},            // 0x60
    {"KP_9", "PGUP"},          // 0x61
    {"KP_0", "INS"},           // 0x62
    {"KP_.", "DEL"},           // 0x63
    {"\\", "|"},               // 0x64 - Non-US \ and |
    {"APPLICATION", nullptr},  // 0x65 - Windows context menu key
    {"POWER", nullptr},        // 0x66
    {"KP_=", nullptr},         // 0x67
    {"F13", nullptr},          // 0x68
    {"F14", nullptr},          // 0x69
    {"F15", nullptr},          // 0x6A
    {"F16", nullptr},          // 0x6B
    {"F17", nullptr},          // 0x6C
    {"F18", nullptr},          // 0x6D
    {"F19", nullptr},          // 0x6E
    {"F20", nullptr},          // 0x6F
    {"F21", nullptr},          // 0x70
    {"F22", nullptr},          // 0x71
    {"F23", nullptr},          // 0x72
    {"F24", nullptr},          // 0x73
    {"EXECUTE", nullptr},      // 0x74
    {"HELP", nullptr},         // 0x75
    {"MENU", nullptr},         // 0x76
    {"SELECT", nullptr},       // 0x77
    {"STOP", nullptr},         // 0x78
    {"AGAIN", nullptr},        // 0x79
    {"UNDO", nullptr},         // 0x7A
    {"CUT", nullptr},          // 0x7B
    {"COPY", nullptr},         // 0x7C
    {"PASTE", nullptr},        // 0x7D
    {"FIND", nullptr},         // 0x7E
    {"MUTE", nullptr},         // 0x7F
    {"VOLUME_UP", nullptr},    // 0x80
    {"VOLUME_DOWN", nullptr},  // 0x81
    {nullptr, nullptr},        // 0x82
    {nullptr, nullptr},        // 0x83
    {nullptr, nullptr},        // 0x84
    {"KP_,", nullptr},         // 0x85
    {"KP_=", nullptr},         // 0x86 - AS/400
    {nullptr, nullptr},        // 0x87-0xFF continue with nullptr for now
};

// Initialize remaining nullptr entries (0x87-0xFF)
// These are less common international and vendor-specific keys

// ========== CONSUMER CONTROL CODES (Page 0x0C) ==========
struct ConsumerKey {
    uint16_t code;
    const char* name;
};

const ConsumerKey PROGMEM CONSUMER_KEYS[] = {
    {0x0000, nullptr},
    {0x0030, "POWER"},
    {0x0040, "MENU"},
    {0x0045, "MENU_UP"},
    {0x0046, "MENU_DOWN"},
    {0x0047, "MENU_LEFT"},
    {0x0048, "MENU_RIGHT"},
    {0x0049, "MENU_ESCAPE"},
    {0x0083, "MEDIA_SELECT"},
    {0x0088, "MEDIA_PC"},
    {0x0089, "MEDIA_TV"},
    {0x008A, "MEDIA_WWW"},
    {0x008B, "MEDIA_DVD"},
    {0x008C, "MEDIA_PHONE"},
    {0x008D, "PROGRAM_GUIDE"},
    {0x008E, "VIDEO_PHONE"},
    {0x0094, "EXIT"},
    {0x009C, "CHANNEL_UP"},
    {0x009D, "CHANNEL_DOWN"},
    {0x00B0, "PLAY"},
    {0x00B1, "PAUSE"},
    {0x00B2, "RECORD"},
    {0x00B3, "FAST_FORWARD"},
    {0x00B4, "REWIND"},
    {0x00B5, "NEXT_TRACK"},
    {0x00B6, "PREV_TRACK"},
    {0x00B7, "STOP"},
    {0x00B8, "EJECT"},
    {0x00CD, "PLAY_PAUSE"},
    {0x00E2, "MUTE"},
    {0x00E9, "VOLUME_UP"},
    {0x00EA, "VOLUME_DOWN"},
    {0x0183, "MEDIA_SELECT_WWW"},
    {0x018A, "MAIL"},
    {0x0192, "CALCULATOR"},
    {0x0194, "MY_COMPUTER"},
    {0x0196, "WWW_SEARCH"},
    {0x0197, "WWW_HOME"},
    {0x0198, "WWW_BACK"},
    {0x0199, "WWW_FORWARD"},
    {0x019A, "WWW_STOP"},
    {0x019B, "WWW_REFRESH"},
    {0x019C, "WWW_FAVORITES"},
    {0x0221, "SEARCH"},
    {0x0223, "HOME"},
    {0x0224, "BACK"},
    {0x0225, "FORWARD"},
    {0x0226, "STOP"},
    {0x0227, "REFRESH"},
    {0x0228, "BOOKMARKS"},
    {0xFFFF, nullptr}  // End marker
};

// ========== SYSTEM CONTROL CODES (Page 0x01) ==========
const char* const PROGMEM SYSTEM_KEYS[] = {
    nullptr,           // 0x00
    "POWER_DOWN",      // 0x01
    "SLEEP",           // 0x02
    "WAKEUP",          // 0x03
    "CONTEXT_MENU",    // 0x04
    "MAIN_MENU",       // 0x05
    "APP_MENU",        // 0x06
    "MENU_HELP",       // 0x07
    "MENU_EXIT",       // 0x08
    "MENU_SELECT",     // 0x09
    "MENU_RIGHT",      // 0x0A
    "MENU_LEFT",       // 0x0B
    "MENU_UP",         // 0x0C
    "MENU_DOWN"        // 0x0D
};

// ========== HELPER FUNCTIONS ==========

// Get key name for US layout
inline const char* getKeyName(uint8_t keycode, bool shifted = false) {
    if (keycode >= 0x87) return nullptr;

    const KeyName* key = &USB_KEYS_US[keycode];
    if (shifted && key->shifted != nullptr) {
        return key->shifted;
    }
    return key->normal;
}

// Get modifier name
inline const char* getModifierName(uint8_t index) {
    if (index >= 8) return nullptr;
    return MOD_NAMES[index];
}

// Get consumer control key name
inline const char* getConsumerKeyName(uint16_t code) {
    for (int i = 0; i < sizeof(CONSUMER_KEYS) / sizeof(ConsumerKey); i++) {
        if (CONSUMER_KEYS[i].code == code) {
            return CONSUMER_KEYS[i].name;
        }
        if (CONSUMER_KEYS[i].code > code) {
            break;  // Codes are sorted
        }
    }
    return nullptr;
}

// Get system control key name
inline const char* getSystemKeyName(uint8_t code) {
    if (code >= sizeof(SYSTEM_KEYS) / sizeof(char*)) {
        return nullptr;
    }
    return SYSTEM_KEYS[code];
}

// Get friendly layout name
inline const char* getLayoutName(uint8_t countryCode) {
    switch (countryCode) {
        case 0: return "US QWERTY";
        case 6: return "UK QWERTY";
        case 8: return "French AZERTY";
        case 9: return "German QWERTZ";
        case 10: return "Spanish";
        case 11: return "Italian";
        case 15: return "Japanese JIS";
        default: return "Unknown Layout";
    }
}

#endif // HID_KEYDEFS_H
