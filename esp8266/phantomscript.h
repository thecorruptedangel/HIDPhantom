/*
 * phantomscript.h
 *
 * PhantomScript Engine - DuckyScript 3.0 Compatible Script Parser and Executor
 *
 * A complete implementation of DuckyScript 3.0 syntax for ESP8266, designed to
 * parse and execute keyboard automation scripts with support for variables,
 * control flow, functions, and dynamic HID report generation.
 *
 * Inspired by the open-source rasper-ducky project (https://github.com/leogout/rasper-ducky)
 *
 * Architecture:
 *   1. Tokenizer - Splits script into meaningful tokens
 *   2. Parser - Builds execution context with variables/functions/control flow
 *   3. Executor - Executes commands and manages state
 *   4. HID Generator - Converts commands to raw HID reports for RP2040
 *
 * Features:
 *   - Variables: $var = 10, $name = "text"
 *   - Constants: DEFINE #COUNT 5
 *   - Control Flow: IF/ELSE/END_IF, WHILE/END_WHILE
 *   - Functions: FUNCTION name ... END_FUNCTION
 *   - Expressions: $x + 2 * 3, $a == $b, $c && $d
 *   - Random: RANDOM_LOWERCASE_LETTER, RANDOM_NUMBER, etc.
 *   - String Typing: STRING, STRINGLN with layout support
 *   - Keys: All standard keys, modifiers, function keys
 *   - Delays: DELAY milliseconds
 *
 * Usage:
 *   PhantomScript script;
 *   script.load("/script.txt");  // Load from LittleFS
 *   script.execute(sendHIDCommand);  // Execute with HID callback
 */

#ifndef PHANTOMSCRIPT_H
#define PHANTOMSCRIPT_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>

// Default limits for safety (can be customized via settings)
#define PS_DEFAULT_MAX_VARIABLES 50
#define PS_DEFAULT_MAX_CONSTANTS 30
#define PS_DEFAULT_MAX_FUNCTIONS 10
#define PS_DEFAULT_MAX_LOOP_ITERATIONS 10000
#define PS_DEFAULT_MAX_CALL_DEPTH 5  // Conservative for ESP8266 stack
#define PS_DEFAULT_MAX_LINE_LENGTH 256
#define PS_DEFAULT_MAX_STRING_LENGTH 512
#define PS_DEFAULT_MAX_SCRIPT_LINES 1000
#define PS_DEFAULT_MAX_EXECUTION_TIME 300000  // 5 minutes in milliseconds

// Runtime configurable safety limits structure
struct PSScriptSettings {
    int maxVariables;
    int maxConstants;
    int maxFunctions;
    int maxLoopIterations;
    int maxCallDepth;
    int maxLineLength;
    int maxStringLength;
    int maxScriptLines;
    int maxExecutionTime;
    int defaultInterKeystrokeDelay;  // Default delay between keystrokes in ms (if no DEFAULTDELAY command)

    PSScriptSettings() {
        resetToDefaults();
    }

    void resetToDefaults() {
        maxVariables = PS_DEFAULT_MAX_VARIABLES;
        maxConstants = PS_DEFAULT_MAX_CONSTANTS;
        maxFunctions = PS_DEFAULT_MAX_FUNCTIONS;
        maxLoopIterations = PS_DEFAULT_MAX_LOOP_ITERATIONS;
        maxCallDepth = PS_DEFAULT_MAX_CALL_DEPTH;
        maxLineLength = PS_DEFAULT_MAX_LINE_LENGTH;
        maxStringLength = PS_DEFAULT_MAX_STRING_LENGTH;
        maxScriptLines = PS_DEFAULT_MAX_SCRIPT_LINES;
        maxExecutionTime = PS_DEFAULT_MAX_EXECUTION_TIME;
        defaultInterKeystrokeDelay = 4;  // 10ms default (safe for most systems)
    }
};

// HID keyboard report structure (8 bytes)
struct HIDKeyboardReport {
    uint8_t modifiers;    // Bit flags for modifier keys
    uint8_t reserved;     // Reserved byte (always 0)
    uint8_t keys[6];      // Up to 6 simultaneous key presses
};

// Forward declaration
class PhantomScript;

// Callback type for sending HID commands to RP2040 via UART
// Format: "HID:01:0008:MMRRKKKKKKKKKKKKKKK\r\n"
typedef std::function<void(const char* hidCommand)> HIDCallback;

// USB HID Keyboard Scancodes (from HID Usage Tables)
enum HIDKeycode {
    KEY_NONE = 0x00,
    KEY_A = 0x04,
    KEY_B = 0x05,
    KEY_C = 0x06,
    KEY_D = 0x07,
    KEY_E = 0x08,
    KEY_F = 0x09,
    KEY_G = 0x0A,
    KEY_H = 0x0B,
    KEY_I = 0x0C,
    KEY_J = 0x0D,
    KEY_K = 0x0E,
    KEY_L = 0x0F,
    KEY_M = 0x10,
    KEY_N = 0x11,
    KEY_O = 0x12,
    KEY_P = 0x13,
    KEY_Q = 0x14,
    KEY_R = 0x15,
    KEY_S = 0x16,
    KEY_T = 0x17,
    KEY_U = 0x18,
    KEY_V = 0x19,
    KEY_W = 0x1A,
    KEY_X = 0x1B,
    KEY_Y = 0x1C,
    KEY_Z = 0x1D,
    KEY_1 = 0x1E,
    KEY_2 = 0x1F,
    KEY_3 = 0x20,
    KEY_4 = 0x21,
    KEY_5 = 0x22,
    KEY_6 = 0x23,
    KEY_7 = 0x24,
    KEY_8 = 0x25,
    KEY_9 = 0x26,
    KEY_0 = 0x27,
    KEY_ENTER = 0x28,
    KEY_ESC = 0x29,
    KEY_BACKSPACE = 0x2A,
    KEY_TAB = 0x2B,
    KEY_SPACE = 0x2C,
    KEY_MINUS = 0x2D,
    KEY_EQUAL = 0x2E,
    KEY_LEFTBRACE = 0x2F,
    KEY_RIGHTBRACE = 0x30,
    KEY_BACKSLASH = 0x31,
    KEY_SEMICOLON = 0x33,
    KEY_APOSTROPHE = 0x34,
    KEY_GRAVE = 0x35,
    KEY_COMMA = 0x36,
    KEY_DOT = 0x37,
    KEY_SLASH = 0x38,
    KEY_CAPSLOCK = 0x39,
    KEY_F1 = 0x3A,
    KEY_F2 = 0x3B,
    KEY_F3 = 0x3C,
    KEY_F4 = 0x3D,
    KEY_F5 = 0x3E,
    KEY_F6 = 0x3F,
    KEY_F7 = 0x40,
    KEY_F8 = 0x41,
    KEY_F9 = 0x42,
    KEY_F10 = 0x43,
    KEY_F11 = 0x44,
    KEY_F12 = 0x45,
    KEY_PRINTSCREEN = 0x46,
    KEY_SCROLLLOCK = 0x47,
    KEY_PAUSE = 0x48,
    KEY_INSERT = 0x49,
    KEY_HOME = 0x4A,
    KEY_PAGEUP = 0x4B,
    KEY_DELETE = 0x4C,
    KEY_END = 0x4D,
    KEY_PAGEDOWN = 0x4E,
    KEY_RIGHT = 0x4F,
    KEY_LEFT = 0x50,
    KEY_DOWN = 0x51,
    KEY_UP = 0x52,
    KEY_NUMLOCK = 0x53,
    KEY_KPSLASH = 0x54,
    KEY_KPASTERISK = 0x55,
    KEY_KPMINUS = 0x56,
    KEY_KPPLUS = 0x57,
    KEY_KPENTER = 0x58,
    KEY_KP1 = 0x59,
    KEY_KP2 = 0x5A,
    KEY_KP3 = 0x5B,
    KEY_KP4 = 0x5C,
    KEY_KP5 = 0x5D,
    KEY_KP6 = 0x5E,
    KEY_KP7 = 0x5F,
    KEY_KP8 = 0x60,
    KEY_KP9 = 0x61,
    KEY_KP0 = 0x62,
    KEY_KPDOT = 0x63,
    KEY_APPLICATION = 0x65,
    KEY_MENU = 0x76,
};

// Modifier key bitmasks (matching hid_keydefs.h)
#define MOD_LCTRL   0x01
#define MOD_LSHIFT  0x02
#define MOD_LALT    0x04
#define MOD_LGUI    0x08
#define MOD_RCTRL   0x10
#define MOD_RSHIFT  0x20
#define MOD_RALT    0x40
#define MOD_RGUI    0x80

// Token types for parsing
enum TokenType {
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_VARIABLE,     // $name
    TOKEN_CONSTANT,     // #NAME
    TOKEN_OPERATOR,     // + - * / == != < > <= >= && || !
    TOKEN_ASSIGN,       // =
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
};

// Token structure
struct Token {
    TokenType type;
    String value;
    int lineNumber;
};

// Variable value (can be int or string)
struct Variable {
    bool isString;
    int intValue;
    String stringValue;

    Variable() : isString(false), intValue(0) {}
    Variable(int val) : isString(false), intValue(val) {}
    Variable(const String& val) : isString(true), intValue(0), stringValue(val) {}
};

// Function definition
struct FunctionDef {
    String name;
    int startLine;
    int endLine;
};

// Execution context
struct ExecutionContext {
    std::map<String, Variable> variables;
    std::map<String, int> constants;
    std::map<String, FunctionDef> functions;
    int currentLine;
    int callDepth;
    int totalIterations;  // Track all loop iterations globally
    bool stopExecution;
    bool pauseExecution;
    String errorMessage;
    int defaultDelay;  // Default delay between commands in milliseconds

    ExecutionContext() : currentLine(0), callDepth(0), totalIterations(0), stopExecution(false), pauseExecution(false), defaultDelay(0) {}
};

// PhantomScript Engine
class PhantomScript {
public:
    PhantomScript();
    ~PhantomScript();

    // Load script from file (LittleFS)
    bool load(const char* filename);

    // Load script from string (useful for hardcoded scripts)
    // Example usage:
    //   String script = "STRING Hello World\nDELAY 1000\nENTER";
    //   phantomScript.loadFromString(script);
    //   phantomScript.execute(sendHIDCallback);
    bool loadFromString(const String& scriptContent);

    // Execute the loaded script
    bool execute(HIDCallback hidCallback);

    // Get last error message
    String getError() const { return context.errorMessage; }

    // Check if script is loaded
    bool isLoaded() const { return !lines.empty(); }

    // Get script line count
    int getLineCount() const { return lines.size(); }

    // Stop execution (can be called from callback)
    void stop() { context.stopExecution = true; }

    // Pause execution (can be called from callback)
    void pause() { context.pauseExecution = true; }

    // Resume execution (can be called from callback)
    void resume() { context.pauseExecution = false; }

    // Check if paused
    bool isPaused() const { return context.pauseExecution; }

    // Configure safety limits (call before load/execute)
    void setSettings(const PSScriptSettings& settings) { this->settings = settings; }

    // Get current settings
    PSScriptSettings getSettings() const { return settings; }

private:
    std::vector<String> lines;
    ExecutionContext context;
    HIDCallback hidCallback;
    PSScriptSettings settings;  // Runtime configurable limits

    // Parser methods
    bool parseLine(int lineNum);
    bool parseCommand(const std::vector<Token>& tokens, int lineNum);
    std::vector<Token> tokenize(const String& line, int lineNum);

    // Expression evaluation
    Variable evaluateExpression(const std::vector<Token>& tokens, int& pos);
    Variable evaluateTerm(const std::vector<Token>& tokens, int& pos);
    Variable evaluateFactor(const std::vector<Token>& tokens, int& pos);
    int evaluateCondition(const std::vector<Token>& tokens, int& pos);

    // Command handlers
    bool cmdString(const std::vector<Token>& tokens, bool addEnter);
    bool cmdDelay(const std::vector<Token>& tokens);
    bool cmdKey(const String& keyName, const std::vector<String>& modifiers);
    bool cmdDefine(const std::vector<Token>& tokens);
    bool cmdVariable(const std::vector<Token>& tokens);
    bool cmdIf(const std::vector<Token>& tokens, int& jumpTo);
    bool cmdWhile(const std::vector<Token>& tokens, int& jumpTo);
    bool cmdFunction(const std::vector<Token>& tokens, int& jumpTo);
    bool cmdCallFunction(const String& funcName);
    bool cmdRandom(const String& randomType, const std::vector<Token>& tokens);

    // HID generation
    void sendKeyPress(uint8_t keycode, uint8_t modifiers = 0);
    void sendKeyRelease();
    void typeCharacter(char ch);
    void typeString(const String& text);

    // Helper functions
    uint8_t charToKeycode(char ch, bool& needShift);
    uint8_t nameToKeycode(const String& keyName);
    uint8_t nameToModifier(const String& modName);
    bool isKeyName(const String& name);
    bool isModifierName(const String& name);
    int findMatchingEnd(const String& startKeyword, const String& endKeyword, int fromLine);
    String randomChar(const String& charset);

    // Error handling
    void setError(const String& msg, int lineNum = -1);
};

// ========== IMPLEMENTATION ==========

PhantomScript::PhantomScript() {
    randomSeed(analogRead(0) ^ micros());
}

PhantomScript::~PhantomScript() {
    lines.clear();
}

bool PhantomScript::load(const char* filename) {
    // Check available heap before loading
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 16384) {  // Require at least 16KB free for loading
        setError(String("Insufficient memory for loading (") + freeHeap + " bytes free, need 16384)");
        return false;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        setError(String("Failed to open file: ") + filename);
        return false;
    }

    lines.clear();
    lines.reserve(100);  // Pre-allocate space to reduce fragmentation
    int lineCount = 0;
    while (file.available() && lineCount < settings.maxScriptLines) {
        String line = file.readStringUntil('\n');

        // Enforce line length limit
        if (line.length() > settings.maxLineLength) {
            line = line.substring(0, settings.maxLineLength);
        }

        line.trim();
        lines.push_back(line);
        lineCount++;
    }
    file.close();

    if (lineCount >= settings.maxScriptLines && file.available()) {
        setError(String("Script too large (max ") + settings.maxScriptLines + " lines)");
        return false;
    }

    return true;
}

bool PhantomScript::loadFromString(const String& scriptContent) {
    lines.clear();
    int start = 0;
    int end = scriptContent.indexOf('\n');

    while (end != -1) {
        String line = scriptContent.substring(start, end);
        line.trim();
        lines.push_back(line);
        start = end + 1;
        end = scriptContent.indexOf('\n', start);
    }

    // Add last line if exists
    if (start < scriptContent.length()) {
        String line = scriptContent.substring(start);
        line.trim();
        lines.push_back(line);
    }

    return true;
}

bool PhantomScript::execute(HIDCallback callback) {
    if (!isLoaded()) {
        setError("No script loaded");
        return false;
    }

    if (!callback) {
        setError("No HID callback provided");
        return false;
    }

    // Check available heap before execution
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 8192) {  // Require at least 8KB free
        setError(String("Insufficient memory (") + freeHeap + " bytes free, need 8192)");
        return false;
    }

    hidCallback = callback;
    context = ExecutionContext();  // Reset context

    // First pass: Find all FUNCTIONs and DEFINEs
    for (int i = 0; i < lines.size(); i++) {
        if (lines[i].isEmpty() || lines[i].startsWith("REM ") || lines[i].startsWith("//")) {
            continue;
        }

        std::vector<Token> tokens = tokenize(lines[i], i);
        if (tokens.empty()) continue;

        if (tokens[0].value.equalsIgnoreCase("FUNCTION") && tokens.size() >= 2) {
            String funcName = tokens[1].value;

            // Check function limit
            if (context.functions.find(funcName) == context.functions.end()) {
                if (context.functions.size() >= settings.maxFunctions) {
                    setError("Maximum number of functions exceeded", i);
                    return false;
                }
            }

            int endLine = findMatchingEnd("FUNCTION", "END_FUNCTION", i);
            if (endLine == -1) {
                setError("FUNCTION without END_FUNCTION", i);
                return false;
            }
            FunctionDef func;
            func.name = funcName;
            func.startLine = i + 1;
            func.endLine = endLine - 1;
            context.functions[funcName] = func;
        }
    }

    // Second pass: Execute commands
    context.currentLine = 0;
    unsigned long startTime = millis();

    while (context.currentLine < lines.size() && !context.stopExecution) {
        // Check if paused - wait until resumed or stopped
        while (context.pauseExecution && !context.stopExecution) {
            yield();
            delay(10);  // Small delay to prevent tight loop
        }

        // Check if stopped while paused
        if (context.stopExecution) break;

        // Check for execution timeout (handle millis() overflow)
        unsigned long elapsed = millis() - startTime;
        if (elapsed > (unsigned long)settings.maxExecutionTime) {
            setError(String("Script execution timeout (max ") + (settings.maxExecutionTime / 60000) + " minutes)");
            return false;
        }

        if (!parseLine(context.currentLine)) {
            return false;
        }
        context.currentLine++;

        // Safety check for infinite loops
        static unsigned long lastYield = 0;
        if (millis() - lastYield > 100) {
            yield();
            lastYield = millis();
        }
    }

    return !context.stopExecution || context.errorMessage.isEmpty();
}

bool PhantomScript::parseLine(int lineNum) {
    if (lineNum < 0 || lineNum >= lines.size()) {
        return true;  // Skip out of bounds
    }

    String line = lines[lineNum];
    if (line.isEmpty() || line.startsWith("REM ") || line.startsWith("//")) {
        return true;  // Skip comments and empty lines
    }

    std::vector<Token> tokens = tokenize(line, lineNum);
    if (tokens.empty()) return true;

    return parseCommand(tokens, lineNum);
}

std::vector<Token> PhantomScript::tokenize(const String& line, int lineNum) {
    std::vector<Token> tokens;
    int i = 0;
    int len = line.length();

    while (i < len) {
        // Skip whitespace
        while (i < len && isspace(line[i])) i++;
        if (i >= len) break;

        Token token;
        token.lineNumber = lineNum;

        // String literal
        if (line[i] == '"') {
            i++;
            String str = "";
            while (i < len && line[i] != '"') {
                // Enforce string length limit
                if (str.length() >= settings.maxStringLength) {
                    // Truncate and close string
                    break;
                }

                if (line[i] == '\\' && i + 1 < len) {
                    i++;
                    if (line[i] == 'n') str += '\n';
                    else if (line[i] == 't') str += '\t';
                    else if (line[i] == 'r') str += '\r';
                    else str += line[i];
                } else {
                    str += line[i];
                }
                i++;
            }
            if (i < len && line[i] == '"') {
                i++;  // Skip closing "
            }
            token.type = TOKEN_STRING;
            token.value = str;
            tokens.push_back(token);
            continue;
        }

        // Variable ($name)
        if (line[i] == '$') {
            i++;
            String varName = "";
            while (i < len && (isalnum(line[i]) || line[i] == '_')) {
                varName += line[i++];
            }
            token.type = TOKEN_VARIABLE;
            token.value = varName;
            tokens.push_back(token);
            continue;
        }

        // Constant (#NAME)
        if (line[i] == '#') {
            i++;
            String constName = "";
            while (i < len && (isalnum(line[i]) || line[i] == '_')) {
                constName += line[i++];
            }
            token.type = TOKEN_CONSTANT;
            token.value = constName;
            tokens.push_back(token);
            continue;
        }

        // Number
        if (isdigit(line[i]) || (line[i] == '-' && i + 1 < len && isdigit(line[i + 1]))) {
            String num = "";
            if (line[i] == '-') num += line[i++];
            while (i < len && isdigit(line[i])) {
                num += line[i++];
            }
            token.type = TOKEN_NUMBER;
            token.value = num;
            tokens.push_back(token);
            continue;
        }

        // Operators
        if (line[i] == '=' && i + 1 < len && line[i + 1] == '=') {
            token.type = TOKEN_OPERATOR;
            token.value = "==";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '!' && i + 1 < len && line[i + 1] == '=') {
            token.type = TOKEN_OPERATOR;
            token.value = "!=";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '<' && i + 1 < len && line[i + 1] == '=') {
            token.type = TOKEN_OPERATOR;
            token.value = "<=";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '>' && i + 1 < len && line[i + 1] == '=') {
            token.type = TOKEN_OPERATOR;
            token.value = ">=";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '&' && i + 1 < len && line[i + 1] == '&') {
            token.type = TOKEN_OPERATOR;
            token.value = "&&";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '|' && i + 1 < len && line[i + 1] == '|') {
            token.type = TOKEN_OPERATOR;
            token.value = "||";
            tokens.push_back(token);
            i += 2;
            continue;
        }
        if (line[i] == '=') {
            token.type = TOKEN_ASSIGN;
            token.value = "=";
            tokens.push_back(token);
            i++;
            continue;
        }
        if (strchr("+-*/<>!", line[i])) {
            token.type = TOKEN_OPERATOR;
            token.value = String(line[i]);
            tokens.push_back(token);
            i++;
            continue;
        }

        // Parentheses
        if (line[i] == '(') {
            token.type = TOKEN_LPAREN;
            token.value = "(";
            tokens.push_back(token);
            i++;
            continue;
        }
        if (line[i] == ')') {
            token.type = TOKEN_RPAREN;
            token.value = ")";
            tokens.push_back(token);
            i++;
            continue;
        }

        // Comma
        if (line[i] == ',') {
            token.type = TOKEN_COMMA;
            token.value = ",";
            tokens.push_back(token);
            i++;
            continue;
        }

        // Identifier (command or key name)
        if (isalpha(line[i]) || line[i] == '_') {
            String ident = "";
            while (i < len && (isalnum(line[i]) || line[i] == '_')) {
                ident += line[i++];
            }
            token.type = TOKEN_IDENTIFIER;
            token.value = ident;
            tokens.push_back(token);
            continue;
        }

        // Unknown character - skip
        i++;
    }

    return tokens;
}

bool PhantomScript::parseCommand(const std::vector<Token>& tokens, int lineNum) {
    if (tokens.empty()) return true;

    String cmd = tokens[0].value;
    cmd.toUpperCase();

    // Comments
    if (cmd == "REM") return true;

    // DEFINE
    if (cmd == "DEFINE") {
        return cmdDefine(tokens);
    }

    // DEFAULTDELAY or DEFAULT_DELAY
    if (cmd == "DEFAULTDELAY" || cmd == "DEFAULT_DELAY") {
        if (tokens.size() >= 2) {
            int pos = 1;
            Variable value = evaluateExpression(tokens, pos);
            if (!value.isString) {
                context.defaultDelay = value.intValue;
                return true;
            }
        }
        setError("DEFAULTDELAY requires numeric value", lineNum);
        return false;
    }

    // FUNCTION (skip - already parsed)
    if (cmd == "FUNCTION" || cmd == "END_FUNCTION") {
        return true;
    }

    // Variable assignment
    if (tokens[0].type == TOKEN_VARIABLE && tokens.size() >= 3 && tokens[1].type == TOKEN_ASSIGN) {
        return cmdVariable(tokens);
    }

    // STRING / STRINGLN
    if (cmd == "STRING" || cmd == "STRINGLN") {
        return cmdString(tokens, cmd == "STRINGLN");
    }

    // DELAY
    if (cmd == "DELAY") {
        return cmdDelay(tokens);
    }

    // IF / ELSE / END_IF
    if (cmd == "IF") {
        int jumpTo = -1;
        return cmdIf(tokens, jumpTo);
    }
    if (cmd == "ELSE" || cmd == "END_IF") {
        return true;  // Handled by IF
    }

    // WHILE / END_WHILE
    if (cmd == "WHILE") {
        int jumpTo = -1;
        return cmdWhile(tokens, jumpTo);
    }
    if (cmd == "END_WHILE") {
        return true;  // Handled by WHILE
    }

    // RANDOM commands
    if (cmd.startsWith("RANDOM_")) {
        return cmdRandom(cmd, tokens);
    }

    // Function call
    if (context.functions.find(cmd) != context.functions.end()) {
        return cmdCallFunction(cmd);
    }

    // Key press (simple key or modifier+key)
    std::vector<String> parts;
    for (const Token& t : tokens) {
        if (t.type == TOKEN_IDENTIFIER) {
            parts.push_back(t.value);
        }
    }

    if (!parts.empty()) {
        // Check if it's a key command
        bool hasKey = false;
        std::vector<String> modifiers;
        String keyName;

        for (const String& part : parts) {
            if (isModifierName(part)) {
                modifiers.push_back(part);
            } else if (isKeyName(part)) {
                keyName = part;
                hasKey = true;
                break;
            }
        }

        if (hasKey) {
            return cmdKey(keyName, modifiers);
        }
    }

    setError("Unknown command: " + cmd, lineNum);
    return false;
}

bool PhantomScript::cmdDefine(const std::vector<Token>& tokens) {
    if (tokens.size() < 3 || tokens[1].type != TOKEN_CONSTANT) {
        setError("Invalid DEFINE syntax", tokens[0].lineNumber);
        return false;
    }

    String constName = tokens[1].value;

    // Check if adding new constant exceeds limit
    if (context.constants.find(constName) == context.constants.end()) {
        if (context.constants.size() >= settings.maxConstants) {
            setError("Maximum number of constants exceeded", tokens[0].lineNumber);
            return false;
        }
    }

    int pos = 2;
    Variable value = evaluateExpression(tokens, pos);

    if (!value.isString) {
        context.constants[constName] = value.intValue;
        return true;
    }

    setError("DEFINE requires numeric value", tokens[0].lineNumber);
    return false;
}

bool PhantomScript::cmdVariable(const std::vector<Token>& tokens) {
    String varName = tokens[0].value;

    // Check if adding new variable exceeds limit
    if (context.variables.find(varName) == context.variables.end()) {
        if (context.variables.size() >= settings.maxVariables) {
            setError("Maximum number of variables exceeded", tokens[0].lineNumber);
            return false;
        }
    }

    int pos = 2;
    Variable value = evaluateExpression(tokens, pos);
    context.variables[varName] = value;
    return true;
}

bool PhantomScript::cmdString(const std::vector<Token>& tokens, bool addEnter) {
    // For STRING command, we need to get the raw text from the original line
    // to preserve all special characters, quotes, and backslashes exactly as typed
    String lineText = lines[context.currentLine];

    // Find "STRING" or "STRINGLN" command and extract everything after it
    String cmd = tokens[0].value;
    cmd.toUpperCase();

    int cmdStart = lineText.indexOf(tokens[0].value);
    if (cmdStart == -1) {
        // Try case-insensitive search
        String upperLine = lineText;
        upperLine.toUpperCase();
        cmdStart = upperLine.indexOf(cmd);
    }

    if (cmdStart == -1) {
        return false;  // Command not found in line (shouldn't happen)
    }

    // Skip past the command name and any whitespace
    int textStart = cmdStart + tokens[0].value.length();
    while (textStart < lineText.length() && isspace(lineText[textStart])) {
        textStart++;
    }

    // Get the raw text (everything after the command)
    String text = lineText.substring(textStart);

    // Handle variables and constants by doing simple string replacement
    // This allows: STRING Hello $name, you have $count items
    int pos = 0;
    while (pos < text.length()) {
        if (text[pos] == '$') {
            // Found variable
            int varStart = pos + 1;
            int varEnd = varStart;
            while (varEnd < text.length() && (isalnum(text[varEnd]) || text[varEnd] == '_')) {
                varEnd++;
            }
            String varName = text.substring(varStart, varEnd);

            if (context.variables.find(varName) != context.variables.end()) {
                Variable& var = context.variables[varName];
                String replacement = var.isString ? var.stringValue : String(var.intValue);
                text = text.substring(0, pos) + replacement + text.substring(varEnd);
                pos += replacement.length();
            } else {
                pos = varEnd;
            }
        } else if (text[pos] == '#') {
            // Found constant
            int constStart = pos + 1;
            int constEnd = constStart;
            while (constEnd < text.length() && (isalnum(text[constEnd]) || text[constEnd] == '_')) {
                constEnd++;
            }
            String constName = text.substring(constStart, constEnd);

            if (context.constants.find(constName) != context.constants.end()) {
                String replacement = String(context.constants[constName]);
                text = text.substring(0, pos) + replacement + text.substring(constEnd);
                pos += replacement.length();
            } else {
                pos = constEnd;
            }
        } else {
            pos++;
        }
    }

    typeString(text);
    if (addEnter) {
        sendKeyPress(KEY_ENTER);
        delay(20);
        sendKeyRelease();

        // Apply default delay after ENTER
        if (context.defaultDelay > 0) {
            delay(context.defaultDelay);
        }
    }

    return true;
}

bool PhantomScript::cmdDelay(const std::vector<Token>& tokens) {
    if (tokens.size() < 2) {
        setError("DELAY requires milliseconds", tokens[0].lineNumber);
        return false;
    }

    int pos = 1;
    Variable value = evaluateExpression(tokens, pos);

    if (!value.isString) {
        delay(value.intValue);
        return true;
    }

    setError("DELAY requires numeric value", tokens[0].lineNumber);
    return false;
}

bool PhantomScript::cmdKey(const String& keyName, const std::vector<String>& modifiers) {
    uint8_t keycode = nameToKeycode(keyName);
    uint8_t mods = 0;

    for (const String& mod : modifiers) {
        mods |= nameToModifier(mod);
    }

    sendKeyPress(keycode, mods);
    delay(20);  // Hold key for 20ms
    sendKeyRelease();

    // Apply default delay after key release
    if (context.defaultDelay > 0) {
        delay(context.defaultDelay);
    }

    return true;
}

bool PhantomScript::cmdIf(const std::vector<Token>& tokens, int& jumpTo) {
    int pos = 1;
    int condition = evaluateCondition(tokens, pos);

    int endLine = findMatchingEnd("IF", "END_IF", context.currentLine);
    if (endLine == -1) {
        setError("IF without END_IF", context.currentLine);
        return false;
    }

    // Find ELSE if exists
    int elseLine = -1;
    for (int i = context.currentLine + 1; i < endLine; i++) {
        std::vector<Token> t = tokenize(lines[i], i);
        if (!t.empty() && t[0].value.equalsIgnoreCase("ELSE")) {
            elseLine = i;
            break;
        }
    }

    if (condition) {
        // Execute IF block
        int endBlock = (elseLine != -1) ? elseLine : endLine;
        for (int i = context.currentLine + 1; i < endBlock; i++) {
            context.currentLine = i;
            if (!parseLine(i)) return false;
        }
        context.currentLine = endLine;
    } else if (elseLine != -1) {
        // Execute ELSE block
        for (int i = elseLine + 1; i < endLine; i++) {
            context.currentLine = i;
            if (!parseLine(i)) return false;
        }
        context.currentLine = endLine;
    } else {
        // Skip to END_IF
        context.currentLine = endLine;
    }

    return true;
}

bool PhantomScript::cmdWhile(const std::vector<Token>& tokens, int& jumpTo) {
    int whileStart = context.currentLine;
    int endLine = findMatchingEnd("WHILE", "END_WHILE", whileStart);

    if (endLine == -1) {
        setError("WHILE without END_WHILE", whileStart);
        return false;
    }

    int iterations = 0;
    while (iterations < settings.maxLoopIterations && context.totalIterations < settings.maxLoopIterations) {
        int pos = 1;
        int condition = evaluateCondition(tokens, pos);

        if (!condition) {
            context.currentLine = endLine;
            break;
        }

        // Execute loop body
        for (int i = whileStart + 1; i < endLine; i++) {
            context.currentLine = i;
            if (!parseLine(i)) return false;
        }

        iterations++;
        context.totalIterations++;
    }

    if (iterations >= settings.maxLoopIterations || context.totalIterations >= settings.maxLoopIterations) {
        setError("WHILE loop exceeded max iterations", whileStart);
        return false;
    }

    return true;
}

bool PhantomScript::cmdFunction(const std::vector<Token>& tokens, int& jumpTo) {
    // Skip to END_FUNCTION
    jumpTo = findMatchingEnd("FUNCTION", "END_FUNCTION", context.currentLine);
    return true;
}

bool PhantomScript::cmdCallFunction(const String& funcName) {
    if (context.functions.find(funcName) == context.functions.end()) {
        setError("Function not found: " + funcName, context.currentLine);
        return false;
    }

    if (context.callDepth >= settings.maxCallDepth) {
        setError("Maximum call depth exceeded", context.currentLine);
        return false;
    }

    FunctionDef& func = context.functions[funcName];
    int returnLine = context.currentLine;

    context.callDepth++;
    for (int i = func.startLine; i <= func.endLine; i++) {
        context.currentLine = i;
        if (!parseLine(i)) {
            context.callDepth--;
            return false;
        }
    }
    context.callDepth--;

    context.currentLine = returnLine;
    return true;
}

bool PhantomScript::cmdRandom(const String& randomType, const std::vector<Token>& tokens) {
    String charset = "";

    if (randomType == "RANDOM_LOWERCASE_LETTER") {
        charset = "abcdefghijklmnopqrstuvwxyz";
    } else if (randomType == "RANDOM_UPPERCASE_LETTER") {
        charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    } else if (randomType == "RANDOM_LETTER") {
        charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    } else if (randomType == "RANDOM_NUMBER") {
        charset = "0123456789";
    } else if (randomType == "RANDOM_SPECIAL") {
        charset = "!@#$%^&*()_+-=[]{}|;:',.<>?/";
    } else if (randomType == "RANDOM_CHAR") {
        charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:',.<>?/";
    } else if (randomType == "RANDOM_CHAR_FROM" && tokens.size() >= 2 && tokens[1].type == TOKEN_STRING) {
        charset = tokens[1].value;
    } else {
        setError("Unknown random type: " + randomType, tokens[0].lineNumber);
        return false;
    }

    String ch = randomChar(charset);
    typeString(ch);

    return true;
}

Variable PhantomScript::evaluateExpression(const std::vector<Token>& tokens, int& pos) {
    if (pos >= tokens.size()) {
        return Variable(0);
    }

    // Handle string literals
    if (tokens[pos].type == TOKEN_STRING) {
        return Variable(tokens[pos++].value);
    }

    // Handle numbers
    if (tokens[pos].type == TOKEN_NUMBER) {
        int val = tokens[pos++].value.toInt();
        return Variable(val);
    }

    // Handle variables
    if (tokens[pos].type == TOKEN_VARIABLE) {
        String varName = tokens[pos++].value;
        if (context.variables.find(varName) != context.variables.end()) {
            return context.variables[varName];
        }
        return Variable(0);
    }

    // Handle constants
    if (tokens[pos].type == TOKEN_CONSTANT) {
        String constName = tokens[pos++].value;
        if (context.constants.find(constName) != context.constants.end()) {
            return Variable(context.constants[constName]);
        }
        return Variable(0);
    }

    // Handle arithmetic expressions
    return evaluateTerm(tokens, pos);
}

Variable PhantomScript::evaluateTerm(const std::vector<Token>& tokens, int& pos) {
    Variable left = evaluateFactor(tokens, pos);

    while (pos < tokens.size() && tokens[pos].type == TOKEN_OPERATOR) {
        String op = tokens[pos].value;
        if (op != "+" && op != "-") break;
        int opPos = pos;
        pos++;
        Variable right = evaluateFactor(tokens, pos);

        if (!left.isString && !right.isString) {
            if (op == "+") {
                // Check for addition overflow
                if (right.intValue > 0 && left.intValue > INT_MAX - right.intValue) {
                    setError("Integer overflow in addition", tokens[opPos].lineNumber);
                    return Variable(0);
                }
                if (right.intValue < 0 && left.intValue < INT_MIN - right.intValue) {
                    setError("Integer underflow in addition", tokens[opPos].lineNumber);
                    return Variable(0);
                }
                left.intValue += right.intValue;
            }
            else {
                // Check for subtraction overflow
                if (right.intValue < 0 && left.intValue > INT_MAX + right.intValue) {
                    setError("Integer overflow in subtraction", tokens[opPos].lineNumber);
                    return Variable(0);
                }
                if (right.intValue > 0 && left.intValue < INT_MIN + right.intValue) {
                    setError("Integer underflow in subtraction", tokens[opPos].lineNumber);
                    return Variable(0);
                }
                left.intValue -= right.intValue;
            }
        }
    }

    return left;
}

Variable PhantomScript::evaluateFactor(const std::vector<Token>& tokens, int& pos) {
    Variable left;

    if (pos < tokens.size() && tokens[pos].type == TOKEN_LPAREN) {
        pos++;  // Skip (
        left = evaluateExpression(tokens, pos);
        if (pos < tokens.size() && tokens[pos].type == TOKEN_RPAREN) {
            pos++;  // Skip )
        }
        return left;
    }

    // Get primary value
    if (pos < tokens.size()) {
        if (tokens[pos].type == TOKEN_NUMBER) {
            left = Variable(tokens[pos++].value.toInt());
        } else if (tokens[pos].type == TOKEN_VARIABLE) {
            String varName = tokens[pos++].value;
            if (context.variables.find(varName) != context.variables.end()) {
                left = context.variables[varName];
            } else {
                left = Variable(0);
            }
        } else if (tokens[pos].type == TOKEN_CONSTANT) {
            String constName = tokens[pos++].value;
            if (context.constants.find(constName) != context.constants.end()) {
                left = Variable(context.constants[constName]);
            } else {
                left = Variable(0);
            }
        } else {
            return Variable(0);
        }
    }

    while (pos < tokens.size() && tokens[pos].type == TOKEN_OPERATOR) {
        String op = tokens[pos].value;
        if (op != "*" && op != "/") break;
        pos++;

        Variable right;
        if (pos < tokens.size()) {
            if (tokens[pos].type == TOKEN_NUMBER) {
                right = Variable(tokens[pos++].value.toInt());
            } else if (tokens[pos].type == TOKEN_VARIABLE) {
                String varName = tokens[pos++].value;
                if (context.variables.find(varName) != context.variables.end()) {
                    right = context.variables[varName];
                } else {
                    right = Variable(0);
                }
            } else if (tokens[pos].type == TOKEN_CONSTANT) {
                String constName = tokens[pos++].value;
                if (context.constants.find(constName) != context.constants.end()) {
                    right = Variable(context.constants[constName]);
                } else {
                    right = Variable(0);
                }
            }
        }

        if (!left.isString && !right.isString) {
            if (op == "*") {
                // Check for multiplication overflow
                if (right.intValue != 0 && abs(left.intValue) > INT_MAX / abs(right.intValue)) {
                    setError("Integer overflow in multiplication", tokens[0].lineNumber);
                    return Variable(0);
                }
                left.intValue *= right.intValue;
            } else if (op == "/" && right.intValue != 0) {
                // Check for division overflow (INT_MIN / -1 overflows)
                if (left.intValue == INT_MIN && right.intValue == -1) {
                    setError("Integer overflow in division", tokens[0].lineNumber);
                    return Variable(0);
                }
                left.intValue /= right.intValue;
            }
        }
    }

    return left;
}

int PhantomScript::evaluateCondition(const std::vector<Token>& tokens, int& pos) {
    if (pos >= tokens.size()) return 0;

    Variable left = evaluateExpression(tokens, pos);

    if (pos >= tokens.size() || tokens[pos].type != TOKEN_OPERATOR) {
        return left.isString ? 0 : (left.intValue != 0 ? 1 : 0);
    }

    String op = tokens[pos++].value;
    Variable right = evaluateExpression(tokens, pos);

    if (left.isString || right.isString) return 0;

    if (op == "==") return left.intValue == right.intValue ? 1 : 0;
    if (op == "!=") return left.intValue != right.intValue ? 1 : 0;
    if (op == "<") return left.intValue < right.intValue ? 1 : 0;
    if (op == ">") return left.intValue > right.intValue ? 1 : 0;
    if (op == "<=") return left.intValue <= right.intValue ? 1 : 0;
    if (op == ">=") return left.intValue >= right.intValue ? 1 : 0;
    if (op == "&&") return (left.intValue && right.intValue) ? 1 : 0;
    if (op == "||") return (left.intValue || right.intValue) ? 1 : 0;

    return 0;
}

void PhantomScript::sendKeyPress(uint8_t keycode, uint8_t modifiers) {
    HIDKeyboardReport report;
    report.modifiers = modifiers;
    report.reserved = 0;
    report.keys[0] = keycode;
    for (int i = 1; i < 6; i++) report.keys[i] = 0;

    char hidCmd[64];
    snprintf(hidCmd, sizeof(hidCmd),
             "HID:01:0008:%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
             report.modifiers, report.reserved,
             report.keys[0], report.keys[1], report.keys[2],
             report.keys[3], report.keys[4], report.keys[5]);

    hidCallback(hidCmd);
}

void PhantomScript::sendKeyRelease() {
    hidCallback("HID:01:0008:0000000000000000\r\n");
}

void PhantomScript::typeCharacter(char ch) {
    bool needShift = false;
    uint8_t keycode = charToKeycode(ch, needShift);

    if (keycode != KEY_NONE) {
        sendKeyPress(keycode, needShift ? MOD_LSHIFT : 0);
        delay(20);  // Hold key for 20ms
        sendKeyRelease();
        delay(10);  // Wait after release before next character

        // Apply default delay between characters
        // Priority: DEFAULTDELAY command > settings.defaultInterKeystrokeDelay
        int charDelay = (context.defaultDelay > 0) ? context.defaultDelay : settings.defaultInterKeystrokeDelay;
        if (charDelay > 0) {
            delay(charDelay);
        }
    }
}

void PhantomScript::typeString(const String& text) {
    // Enforce string length limit
    int maxLen = min((int)text.length(), settings.maxStringLength);

    for (int i = 0; i < maxLen; i++) {
        if (context.stopExecution) break;  // Allow stopping during long strings
        typeCharacter(text[i]);
    }
}

uint8_t PhantomScript::charToKeycode(char ch, bool& needShift) {
    needShift = false;

    // Validate input (printable ASCII range)
    if (ch < 0x20 && ch != '\n' && ch != '\t') return KEY_NONE;
    if (ch > 0x7E && ch != '\n' && ch != '\t') return KEY_NONE;

    if (ch >= 'a' && ch <= 'z') return KEY_A + (ch - 'a');
    if (ch >= 'A' && ch <= 'Z') {
        needShift = true;
        return KEY_A + (ch - 'A');
    }
    if (ch >= '1' && ch <= '9') return KEY_1 + (ch - '1');
    if (ch == '0') return KEY_0;

    // Special characters (US layout)
    switch (ch) {
        case ' ': return KEY_SPACE;
        case '\n': return KEY_ENTER;
        case '\t': return KEY_TAB;
        case '!': needShift = true; return KEY_1;
        case '@': needShift = true; return KEY_2;
        case '#': needShift = true; return KEY_3;
        case '$': needShift = true; return KEY_4;
        case '%': needShift = true; return KEY_5;
        case '^': needShift = true; return KEY_6;
        case '&': needShift = true; return KEY_7;
        case '*': needShift = true; return KEY_8;
        case '(': needShift = true; return KEY_9;
        case ')': needShift = true; return KEY_0;
        case '-': return KEY_MINUS;
        case '_': needShift = true; return KEY_MINUS;
        case '=': return KEY_EQUAL;
        case '+': needShift = true; return KEY_EQUAL;
        case '[': return KEY_LEFTBRACE;
        case '{': needShift = true; return KEY_LEFTBRACE;
        case ']': return KEY_RIGHTBRACE;
        case '}': needShift = true; return KEY_RIGHTBRACE;
        case '\\': return KEY_BACKSLASH;
        case '|': needShift = true; return KEY_BACKSLASH;
        case ';': return KEY_SEMICOLON;
        case ':': needShift = true; return KEY_SEMICOLON;
        case '\'': return KEY_APOSTROPHE;
        case '"': needShift = true; return KEY_APOSTROPHE;
        case '`': return KEY_GRAVE;
        case '~': needShift = true; return KEY_GRAVE;
        case ',': return KEY_COMMA;
        case '<': needShift = true; return KEY_COMMA;
        case '.': return KEY_DOT;
        case '>': needShift = true; return KEY_DOT;
        case '/': return KEY_SLASH;
        case '?': needShift = true; return KEY_SLASH;
        default: return KEY_NONE;
    }
}

uint8_t PhantomScript::nameToKeycode(const String& keyName) {
    String name = keyName;
    name.toUpperCase();

    if (name == "ENTER" || name == "RETURN") return KEY_ENTER;
    if (name == "ESC" || name == "ESCAPE") return KEY_ESC;
    if (name == "BACKSPACE") return KEY_BACKSPACE;
    if (name == "TAB") return KEY_TAB;
    if (name == "SPACE") return KEY_SPACE;
    if (name == "CAPSLOCK" || name == "CAPS_LOCK") return KEY_CAPSLOCK;
    if (name == "PRINTSCREEN") return KEY_PRINTSCREEN;
    if (name == "SCROLLLOCK") return KEY_SCROLLLOCK;
    if (name == "PAUSE") return KEY_PAUSE;
    if (name == "INSERT") return KEY_INSERT;
    if (name == "HOME") return KEY_HOME;
    if (name == "PAGEUP" || name == "PAGE_UP") return KEY_PAGEUP;
    if (name == "DELETE" || name == "DEL") return KEY_DELETE;
    if (name == "END") return KEY_END;
    if (name == "PAGEDOWN" || name == "PAGE_DOWN") return KEY_PAGEDOWN;
    if (name == "RIGHT" || name == "RIGHTARROW" || name == "RIGHT_ARROW") return KEY_RIGHT;
    if (name == "LEFT" || name == "LEFTARROW" || name == "LEFT_ARROW") return KEY_LEFT;
    if (name == "DOWN" || name == "DOWNARROW" || name == "DOWN_ARROW") return KEY_DOWN;
    if (name == "UP" || name == "UPARROW" || name == "UP_ARROW") return KEY_UP;
    if (name == "NUMLOCK") return KEY_NUMLOCK;
    if (name == "MENU" || name == "APP" || name == "APPLICATION") return KEY_APPLICATION;

    // Function keys
    if (name == "F1") return KEY_F1;
    if (name == "F2") return KEY_F2;
    if (name == "F3") return KEY_F3;
    if (name == "F4") return KEY_F4;
    if (name == "F5") return KEY_F5;
    if (name == "F6") return KEY_F6;
    if (name == "F7") return KEY_F7;
    if (name == "F8") return KEY_F8;
    if (name == "F9") return KEY_F9;
    if (name == "F10") return KEY_F10;
    if (name == "F11") return KEY_F11;
    if (name == "F12") return KEY_F12;

    // Single letters
    if (name.length() == 1 && name[0] >= 'A' && name[0] <= 'Z') {
        return KEY_A + (name[0] - 'A');
    }

    return KEY_NONE;
}

uint8_t PhantomScript::nameToModifier(const String& modName) {
    String name = modName;
    name.toUpperCase();

    if (name == "CTRL" || name == "CONTROL" || name == "LCTRL") return MOD_LCTRL;
    if (name == "SHIFT" || name == "LSHIFT") return MOD_LSHIFT;
    if (name == "ALT" || name == "LALT") return MOD_LALT;
    if (name == "GUI" || name == "WINDOWS" || name == "COMMAND" || name == "LGUI") return MOD_LGUI;
    if (name == "RCTRL") return MOD_RCTRL;
    if (name == "RSHIFT") return MOD_RSHIFT;
    if (name == "RALT") return MOD_RALT;
    if (name == "RGUI") return MOD_RGUI;

    return 0;
}

bool PhantomScript::isKeyName(const String& name) {
    return nameToKeycode(name) != KEY_NONE;
}

bool PhantomScript::isModifierName(const String& name) {
    return nameToModifier(name) != 0;
}

int PhantomScript::findMatchingEnd(const String& startKeyword, const String& endKeyword, int fromLine) {
    int depth = 1;
    for (int i = fromLine + 1; i < lines.size(); i++) {
        std::vector<Token> tokens = tokenize(lines[i], i);
        if (tokens.empty()) continue;

        String cmd = tokens[0].value;
        cmd.toUpperCase();

        if (cmd == startKeyword) depth++;
        if (cmd == endKeyword) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

String PhantomScript::randomChar(const String& charset) {
    if (charset.length() == 0) return "";
    int idx = random(charset.length());
    return String(charset[idx]);
}

void PhantomScript::setError(const String& msg, int lineNum) {
    if (lineNum >= 0) {
        context.errorMessage = String("Line ") + (lineNum + 1) + ": " + msg;
    } else {
        context.errorMessage = msg;
    }
    context.stopExecution = true;
}

#endif // PHANTOMSCRIPT_H
