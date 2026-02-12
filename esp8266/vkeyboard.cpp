/*
 * vkeyboard.cpp
 *
 * Virtual QWERTY Keyboard Implementation
 * Manages key state, modifiers, toggles, and HID report generation
 */

#include "vkeyboard.h"

// ========== GLOBAL STATE ==========
static VirtualKeyboardState vkState;

// ========== HELPER FUNCTIONS ==========

// Check if keycode is a modifier key
static bool isModifierKey(uint8_t keycode) {
    return (keycode >= 0xE0 && keycode <= 0xE7);
}

// Check if keycode is a toggle key
static bool isToggleKey(uint8_t keycode) {
    return (keycode == VKEY_CAPSLOCK ||
            keycode == VKEY_NUMLOCK ||
            keycode == VKEY_SCROLLLOCK);
}

// Get modifier bit for a modifier keycode (0xE0-0xE7)
static uint8_t getModifierBit(uint8_t keycode) {
    if (keycode >= 0xE0 && keycode <= 0xE7) {
        return 1 << (keycode - 0xE0);
    }
    return 0;
}

// Get toggle bit for a toggle keycode
static uint8_t getToggleBit(uint8_t keycode) {
    switch (keycode) {
        case VKEY_CAPSLOCK: return VKEY_TOGGLE_CAPS_LOCK;
        case VKEY_NUMLOCK: return VKEY_TOGGLE_NUM_LOCK;
        case VKEY_SCROLLLOCK: return VKEY_TOGGLE_SCROLL_LOCK;
        default: return 0;
    }
}

// Add key to pressed keys array (if not already present)
static bool addKeyToArray(uint8_t keycode) {
    // Check if already pressed
    for (uint8_t i = 0; i < vkState.keyCount; i++) {
        if (vkState.pressedKeys[i] == keycode) {
            return true;  // Already in array
        }
    }

    // Add if space available
    if (vkState.keyCount < VKEY_MAX_KEYS) {
        vkState.pressedKeys[vkState.keyCount] = keycode;
        vkState.keyCount++;
        return true;
    }

    return false;  // Array full (6KRO limit reached)
}

// Remove key from pressed keys array
static bool removeKeyFromArray(uint8_t keycode) {
    for (uint8_t i = 0; i < vkState.keyCount; i++) {
        if (vkState.pressedKeys[i] == keycode) {
            // Shift remaining keys down
            for (uint8_t j = i; j < vkState.keyCount - 1; j++) {
                vkState.pressedKeys[j] = vkState.pressedKeys[j + 1];
            }
            vkState.pressedKeys[vkState.keyCount - 1] = VKEY_NONE;
            vkState.keyCount--;
            return true;
        }
    }
    return false;
}

// ========== PUBLIC API IMPLEMENTATION ==========

void vkeyboard_init() {
    // Clear all state
    vkState.modifiers = 0;
    vkState.toggles = 0;
    vkState.keyCount = 0;

    for (uint8_t i = 0; i < VKEY_MAX_KEYS; i++) {
        vkState.pressedKeys[i] = VKEY_NONE;
    }

    for (uint8_t i = 0; i < 8; i++) {
        vkState.stickyModifiers[i] = false;
    }

    Serial.println("Virtual keyboard initialized");
}

bool vkeyboard_pressKey(uint8_t keycode) {
    if (keycode == VKEY_NONE) return false;

    // Handle modifier keys (0xE0-0xE7: LCtrl, LShift, LAlt, LGui, RCtrl, RShift, RAlt, RGui)
    if (isModifierKey(keycode)) {
        uint8_t modBit = getModifierBit(keycode);
        vkState.modifiers |= modBit;
        return true;
    }

    // Handle toggle keys (Caps Lock, Num Lock, Scroll Lock)
    if (isToggleKey(keycode)) {
        uint8_t toggleBit = getToggleBit(keycode);
        vkState.toggles ^= toggleBit;  // XOR to toggle state

        // Toggle keys are sent as normal keys in HID report
        // They toggle LED state on host, but we still track state locally
        return addKeyToArray(keycode);
    }

    // Handle normal keys
    return addKeyToArray(keycode);
}

bool vkeyboard_releaseKey(uint8_t keycode) {
    if (keycode == VKEY_NONE) return false;

    // Handle modifier keys
    if (isModifierKey(keycode)) {
        uint8_t modBit = getModifierBit(keycode);

        // Check if sticky mode is enabled for this modifier
        uint8_t modIndex = keycode - 0xE0;
        if (vkState.stickyModifiers[modIndex]) {
            // In sticky mode, don't clear on release
            // It will be cleared on next normal key press or explicit toggle
            return true;
        }

        vkState.modifiers &= ~modBit;
        return true;
    }

    // Handle toggle keys - release them from array but keep toggle state
    if (isToggleKey(keycode)) {
        return removeKeyFromArray(keycode);
    }

    // Handle normal keys
    return removeKeyFromArray(keycode);
}

void vkeyboard_releaseAll() {
    vkState.modifiers = 0;
    vkState.keyCount = 0;

    for (uint8_t i = 0; i < VKEY_MAX_KEYS; i++) {
        vkState.pressedKeys[i] = VKEY_NONE;
    }

    for (uint8_t i = 0; i < 8; i++) {
        vkState.stickyModifiers[i] = false;
    }

    // Note: We don't clear toggle states (Caps/Num/Scroll Lock)
    // Those should persist until explicitly toggled
}

uint8_t vkeyboard_getModifiers() {
    return vkState.modifiers;
}

uint8_t vkeyboard_getToggles() {
    return vkState.toggles;
}

bool vkeyboard_isKeyPressed(uint8_t keycode) {
    if (isModifierKey(keycode)) {
        uint8_t modBit = getModifierBit(keycode);
        return (vkState.modifiers & modBit) != 0;
    }

    for (uint8_t i = 0; i < vkState.keyCount; i++) {
        if (vkState.pressedKeys[i] == keycode) {
            return true;
        }
    }

    return false;
}

void vkeyboard_generateReport(uint8_t* report) {
    // Standard USB HID keyboard report format (8 bytes):
    // Byte 0: Modifier byte (bit flags)
    // Byte 1: Reserved (always 0x00)
    // Bytes 2-7: Up to 6 pressed key codes

    report[0] = vkState.modifiers;
    report[1] = 0x00;  // Reserved

    for (uint8_t i = 0; i < VKEY_MAX_KEYS; i++) {
        report[2 + i] = vkState.pressedKeys[i];
    }
}

String vkeyboard_getHIDString() {
    // Generate HID report in UART protocol format: "HID:01:0008:XXXXXXXXXXXX\r\n"
    // Report ID = 0x01 (keyboard)
    // Length = 0x0008 (8 bytes)

    uint8_t report[VKEY_REPORT_SIZE];
    vkeyboard_generateReport(report);

    String hidStr = "HID:01:0008:";

    for (uint8_t i = 0; i < VKEY_REPORT_SIZE; i++) {
        if (report[i] < 0x10) hidStr += "0";
        hidStr += String(report[i], HEX);
    }

    return hidStr;
}

void vkeyboard_printState() {
    Serial.print("Virtual Keyboard State - Modifiers: 0x");
    Serial.print(vkState.modifiers, HEX);
    Serial.print(", Toggles: 0x");
    Serial.print(vkState.toggles, HEX);
    Serial.print(", Keys pressed: ");
    Serial.print(vkState.keyCount);
    Serial.print(" [");

    for (uint8_t i = 0; i < vkState.keyCount; i++) {
        if (i > 0) Serial.print(", ");
        Serial.print("0x");
        Serial.print(vkState.pressedKeys[i], HEX);
    }

    Serial.println("]");
}
