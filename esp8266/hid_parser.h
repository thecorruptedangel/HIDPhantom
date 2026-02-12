/*
 * hid_parser.h
 *
 * USB HID report parser with state tracking
 * Converts raw HID data to human-readable format
 * Prevents spam by only showing state changes
 */

#ifndef HID_PARSER_H
#define HID_PARSER_H

#include <Arduino.h>
#include "hid_keydefs.h"

// Report IDs (must match RP2040 definitions)
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_MOUSE 2
#define REPORT_ID_CONSUMER_CONTROL 3
#define REPORT_ID_GAMEPAD 4
#define REPORT_ID_SYSTEM_CONTROL 5

class HIDParser {
private:
    // ========== STATE TRACKING (PREVENT SPAM) ==========
    struct KeyboardState {
        uint8_t modifier;
        uint8_t keys[6];
    };

    struct MouseState {
        uint8_t buttons;
        int8_t lastX;
        int8_t lastY;
    };

    KeyboardState lastKbdState;
    MouseState lastMouseState;
    uint16_t lastConsumerCode;
    uint8_t lastSystemCode;

    uint8_t currentLayout;

    // ========== HELPER FUNCTIONS ==========

    // Check if key is in array
    bool keyInArray(uint8_t key, const uint8_t* array, uint8_t size) {
        for (uint8_t i = 0; i < size; i++) {
            if (array[i] == key) return true;
        }
        return false;
    }

    // Build modifier string (e.g., "LCTRL+LSHIFT")
    String buildModifierString(uint8_t modifier) {
        String result = "";
        bool first = true;

        for (uint8_t i = 0; i < 8; i++) {
            if (modifier & (1 << i)) {
                if (!first) result += "+";
                result += getModifierName(i);
                first = false;
            }
        }

        return result;
    }

    // Get key name with layout awareness
    String getKeyDisplayName(uint8_t keycode, bool shifted) {
        const char* name = getKeyName(keycode, shifted);
        if (name != nullptr) {
            return String(name);
        }

        // Unknown key
        char buf[16];
        snprintf(buf, sizeof(buf), "KEY_0x%02X", keycode);
        return String(buf);
    }

public:
    HIDParser() {
        reset();
        currentLayout = LAYOUT_US;  // Default to US layout
    }

    // Reset all state (call on device disconnect)
    void reset() {
        memset(&lastKbdState, 0, sizeof(lastKbdState));
        memset(&lastMouseState, 0, sizeof(lastMouseState));
        lastConsumerCode = 0;
        lastSystemCode = 0;
    }

    // Set keyboard layout
    void setLayout(uint8_t layout) {
        currentLayout = layout;
    }

    // ========== KEYBOARD PARSING ==========
    String parseKeyboardReport(const uint8_t* data, uint16_t len) {
        if (len < 8) return "";  // Invalid report

        uint8_t modifier = data[0];
        // data[1] is reserved
        const uint8_t* keys = &data[2];  // 6 key codes

        // Check if state changed
        if (modifier == lastKbdState.modifier &&
            memcmp(keys, lastKbdState.keys, 6) == 0) {
            return "";  // No change, don't spam
        }

        // Build output string
        String output = "";

        // Check for key release (all zeros after modifier)
        bool allReleased = true;
        for (int i = 0; i < 6; i++) {
            if (keys[i] != 0) {
                allReleased = false;
                break;
            }
        }

        if (allReleased && modifier == 0) {
            // All keys released - don't show this
            lastKbdState.modifier = modifier;
            memcpy(lastKbdState.keys, keys, 6);
            return "";
        }

        // Build key combination string
        String combo = "";

        // Add modifiers
        if (modifier != 0) {
            combo = buildModifierString(modifier);
        }

        // Add keys
        bool shifted = (modifier & (MOD_LSHIFT | MOD_RSHIFT)) != 0;
        for (int i = 0; i < 6; i++) {
            if (keys[i] != 0 && keys[i] < 0xE0) {  // Valid key code
                if (combo.length() > 0) combo += "+";
                combo += getKeyDisplayName(keys[i], shifted);
            }
        }

        if (combo.length() > 0) {
            output = "🎹 [" + combo + "]";
        }

        // Update state
        lastKbdState.modifier = modifier;
        memcpy(lastKbdState.keys, keys, 6);

        return output;
    }

    // ========== MOUSE PARSING ==========
    String parseMouseReport(const uint8_t* data, uint16_t len) {
        if (len < 3) return "";  // Invalid report

        uint8_t buttons = data[0];
        int8_t x = (len > 1) ? (int8_t)data[1] : 0;
        int8_t y = (len > 2) ? (int8_t)data[2] : 0;
        int8_t wheel = (len > 3) ? (int8_t)data[3] : 0;

        String output = "";

        // Check button state change
        if (buttons != lastMouseState.buttons) {
            String buttonStr = "";

            if (buttons & 0x01) buttonStr += "LEFT";
            if (buttons & 0x02) {
                if (buttonStr.length() > 0) buttonStr += "+";
                buttonStr += "RIGHT";
            }
            if (buttons & 0x04) {
                if (buttonStr.length() > 0) buttonStr += "+";
                buttonStr += "MIDDLE";
            }
            if (buttons & 0x08) {
                if (buttonStr.length() > 0) buttonStr += "+";
                buttonStr += "BACK";
            }
            if (buttons & 0x10) {
                if (buttonStr.length() > 0) buttonStr += "+";
                buttonStr += "FORWARD";
            }

            if (buttonStr.length() > 0) {
                output += "🖱️ [MOUSE_" + buttonStr + "]";
            }

            lastMouseState.buttons = buttons;
        }

        // Show movement if significant (threshold > 2 pixels)
        if (abs(x) > 2 || abs(y) > 2) {
            if (output.length() > 0) output += " ";

            String movement = "🖱️ [MOVE ";

            if (y < 0) movement += "↑" + String(abs(y));
            else if (y > 0) movement += "↓" + String(y);

            if (x < 0) {
                if (y != 0) movement += " ";
                movement += "← " + String(abs(x));
            } else if (x > 0) {
                if (y != 0) movement += " ";
                movement += "→ " + String(x);
            }

            movement += "]";
            output += movement;
        }

        // Show wheel scroll
        if (wheel != 0) {
            if (output.length() > 0) output += " ";

            if (wheel < 0) {
                output += "🖱️ [SCROLL ↓" + String(abs(wheel)) + "]";
            } else {
                output += "🖱️ [SCROLL ↑" + String(wheel) + "]";
            }
        }

        return output;
    }

    // ========== CONSUMER CONTROL PARSING ==========
    String parseConsumerReport(const uint8_t* data, uint16_t len) {
        if (len < 2) return "";  // Invalid report

        uint16_t code = data[0] | (data[1] << 8);

        // Ignore no-op / release
        if (code == 0) {
            if (lastConsumerCode != 0) {
                lastConsumerCode = 0;
                return "";  // Released, don't show
            }
            return "";
        }

        // Check if same as last
        if (code == lastConsumerCode) {
            return "";  // Don't spam
        }

        lastConsumerCode = code;

        const char* name = getConsumerKeyName(code);
        if (name != nullptr) {
            return String("🎵 [") + name + "]";
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "🎵 [CONSUMER_0x%04X]", code);
            return String(buf);
        }
    }

    // ========== SYSTEM CONTROL PARSING ==========
    String parseSystemReport(const uint8_t* data, uint16_t len) {
        if (len < 1) return "";  // Invalid report

        uint8_t code = data[0];

        // Ignore no-op / release
        if (code == 0) {
            if (lastSystemCode != 0) {
                lastSystemCode = 0;
                return "";  // Released, don't show
            }
            return "";
        }

        // Check if same as last
        if (code == lastSystemCode) {
            return "";  // Don't spam
        }

        lastSystemCode = code;

        const char* name = getSystemKeyName(code);
        if (name != nullptr) {
            return String("⚡ [") + name + "]";
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "⚡ [SYSTEM_0x%02X]", code);
            return String(buf);
        }
    }

    // ========== MAIN PARSE FUNCTION ==========
    String parseHIDReport(uint8_t reportId, const uint8_t* data, uint16_t len) {
        switch (reportId) {
            case REPORT_ID_KEYBOARD:
                return parseKeyboardReport(data, len);

            case REPORT_ID_MOUSE:
                return parseMouseReport(data, len);

            case REPORT_ID_CONSUMER_CONTROL:
                return parseConsumerReport(data, len);

            case REPORT_ID_SYSTEM_CONTROL:
                return parseSystemReport(data, len);

            default:
                // Unknown report ID
                char buf[32];
                snprintf(buf, sizeof(buf), "❓ [REPORT_%02X]", reportId);
                return String(buf);
        }
    }
};

#endif // HID_PARSER_H
