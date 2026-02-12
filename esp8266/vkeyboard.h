/*
 * vkeyboard.h
 *
 * Virtual QWERTY Keyboard Module
 * Handles key state tracking, modifier/toggle management, and HID report generation
 */

#ifndef VKEYBOARD_H
#define VKEYBOARD_H

#include <Arduino.h>

// ========== HID KEYBOARD CONSTANTS ==========

#define VKEY_MAX_KEYS 6          // USB HID supports up to 6 simultaneous keys (6KRO)
#define VKEY_REPORT_SIZE 8       // HID keyboard report is 8 bytes

// Modifier bit masks (byte 0 of HID report)
#define VKEY_MOD_LCTRL   0x01
#define VKEY_MOD_LSHIFT  0x02
#define VKEY_MOD_LALT    0x04
#define VKEY_MOD_LGUI    0x08
#define VKEY_MOD_RCTRL   0x10
#define VKEY_MOD_RSHIFT  0x20
#define VKEY_MOD_RALT    0x40
#define VKEY_MOD_RGUI    0x80

// Toggle key states (internal tracking)
#define VKEY_TOGGLE_CAPS_LOCK    0x01
#define VKEY_TOGGLE_NUM_LOCK     0x02
#define VKEY_TOGGLE_SCROLL_LOCK  0x04

// USB HID Keyboard keycodes (subset for virtual keyboard)
#define VKEY_NONE           0x00
#define VKEY_A              0x04
#define VKEY_B              0x05
#define VKEY_C              0x06
#define VKEY_D              0x07
#define VKEY_E              0x08
#define VKEY_F              0x09
#define VKEY_G              0x0A
#define VKEY_H              0x0B
#define VKEY_I              0x0C
#define VKEY_J              0x0D
#define VKEY_K              0x0E
#define VKEY_L              0x0F
#define VKEY_M              0x10
#define VKEY_N              0x11
#define VKEY_O              0x12
#define VKEY_P              0x13
#define VKEY_Q              0x14
#define VKEY_R              0x15
#define VKEY_S              0x16
#define VKEY_T              0x17
#define VKEY_U              0x18
#define VKEY_V              0x19
#define VKEY_W              0x1A
#define VKEY_X              0x1B
#define VKEY_Y              0x1C
#define VKEY_Z              0x1D
#define VKEY_1              0x1E
#define VKEY_2              0x1F
#define VKEY_3              0x20
#define VKEY_4              0x21
#define VKEY_5              0x22
#define VKEY_6              0x23
#define VKEY_7              0x24
#define VKEY_8              0x25
#define VKEY_9              0x26
#define VKEY_0              0x27
#define VKEY_ENTER          0x28
#define VKEY_ESC            0x29
#define VKEY_BACKSPACE      0x2A
#define VKEY_TAB            0x2B
#define VKEY_SPACE          0x2C
#define VKEY_MINUS          0x2D  // - and _
#define VKEY_EQUAL          0x2E  // = and +
#define VKEY_LEFTBRACE      0x2F  // [ and {
#define VKEY_RIGHTBRACE     0x30  // ] and }
#define VKEY_BACKSLASH      0x31  // \ and |
#define VKEY_SEMICOLON      0x33  // ; and :
#define VKEY_APOSTROPHE     0x34  // ' and "
#define VKEY_GRAVE          0x35  // ` and ~
#define VKEY_COMMA          0x36  // , and <
#define VKEY_DOT            0x37  // . and >
#define VKEY_SLASH          0x38  // / and ?
#define VKEY_CAPSLOCK       0x39
#define VKEY_F1             0x3A
#define VKEY_F2             0x3B
#define VKEY_F3             0x3C
#define VKEY_F4             0x3D
#define VKEY_F5             0x3E
#define VKEY_F6             0x3F
#define VKEY_F7             0x40
#define VKEY_F8             0x41
#define VKEY_F9             0x42
#define VKEY_F10            0x43
#define VKEY_F11            0x44
#define VKEY_F12            0x45
#define VKEY_PRINTSCREEN    0x46
#define VKEY_SCROLLLOCK     0x47
#define VKEY_PAUSE          0x48
#define VKEY_INSERT         0x49
#define VKEY_HOME           0x4A
#define VKEY_PAGEUP         0x4B
#define VKEY_DELETE         0x4C
#define VKEY_END            0x4D
#define VKEY_PAGEDOWN       0x4E
#define VKEY_RIGHT          0x4F
#define VKEY_LEFT           0x50
#define VKEY_DOWN           0x51
#define VKEY_UP             0x52
#define VKEY_NUMLOCK        0x53
#define VKEY_APPLICATION    0x65  // Context menu key

// ========== VIRTUAL KEYBOARD STATE ==========

struct VirtualKeyboardState {
    uint8_t modifiers;              // Current modifier byte (Ctrl, Shift, Alt, GUI)
    uint8_t toggles;                // Toggle key states (Caps, Num, Scroll Lock)
    uint8_t pressedKeys[VKEY_MAX_KEYS];  // Array of currently pressed key codes
    uint8_t keyCount;               // Number of keys currently pressed
    bool stickyModifiers[8];        // Sticky state for each modifier (click to toggle)
};

// ========== FUNCTION PROTOTYPES ==========

// Initialization
void vkeyboard_init();

// Key press/release (called from WebSocket handler)
bool vkeyboard_pressKey(uint8_t keycode);
bool vkeyboard_releaseKey(uint8_t keycode);
void vkeyboard_releaseAll();

// State queries
uint8_t vkeyboard_getModifiers();
uint8_t vkeyboard_getToggles();
bool vkeyboard_isKeyPressed(uint8_t keycode);

// HID report generation
void vkeyboard_generateReport(uint8_t* report);  // Fills 8-byte buffer
String vkeyboard_getHIDString();                 // Returns "HID:01:0008:XXXX..." format

// Debugging
void vkeyboard_printState();

#endif // VKEYBOARD_H
