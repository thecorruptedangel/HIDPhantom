#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Updater.h>
#include "webpage.h"
#include "hid_keydefs.h"
#include "hid_parser.h"
#include "vkeyboard.h"
#include "phantomscript.h"

// lwIP includes for network packet forwarding
extern "C" {
  #include <lwip/netif.h>
  #include <lwip/pbuf.h>
  #include <netif/ethernet.h>  // Use netif/ethernet.h for ESP8266 Arduino core
}

// ========== FIRMWARE VERSION ==========
#define FIRMWARE_VERSION "1.4"

// ========== CONFIGURATION ==========
char AP_SSID[33] = "HID Phantom";  // Max 32 chars + null terminator
char AP_PASSWORD[64] = "hidphantom";  // Max 63 chars + null terminator
const int BAUD_RATE = 921600;
const int SERIAL_RX_BUFFER_SIZE = 2048;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

const int WS_BUFFER_SIZE = 1024;
const unsigned long WS_SEND_INTERVAL = 50;

const char* RECORDING_FILE = "/recording.dat";
const int MAX_RECORDING_ENTRIES = 5000;

// PhantomScript files - Simple single script system
const char* SCRIPT_FILE = "/script.txt";
const char* SCRIPT_SETTINGS_FILE = "/script_settings.dat";

// OTA files
const char* OTA_FIRMWARE_FILE = "/firmware.bin";
const char* OTA_FLAG_FILE = "/ota_flag";

// AP Config file
const char* AP_CONFIG_FILE = "/ap_config.txt";

// Factory Reset Configuration
const int FACTORY_RESET_PIN = 12;  // GPIO12 (D6) - Pull to GND for factory reset
const int LED_PIN = 2;  // GPIO2 - Built-in LED (active LOW on most ESP8266)
const unsigned long SETTINGS_RESET_TIME = 5000;  // 5 seconds - Settings reset only
const unsigned long FULL_RESET_TIME = 10000;     // 10 seconds - Settings + firmware restore

// Factory firmware backup
const char* FACTORY_FIRMWARE_FILE = "/factory_firmware.bin";


// ========== GLOBAL OBJECTS ==========
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");          // Main WebSocket for text commands (HID, AT, recording)
AsyncWebSocket wsImage("/ws_image"); // Dedicated WebSocket for binary image streaming
DNSServer dnsServer;
HIDParser hidParser;
File recordingFile;
PhantomScript phantomScript;

// ========== RECORDING STATE ==========
enum RecordingState {
    REC_IDLE = 0,
    REC_RECORDING = 1,
    REC_STOPPED = 2,
    REC_REPLAYING = 3
};

RecordingState recState = REC_IDLE;
uint32_t recordingStartTime = 0;
int recordingEntryCount = 0;

// ========== HYBRID MODE STATE ==========
bool hybridModeEnabled = false;

// ========== SERIAL CDC MODE STATE ==========
bool serialModeEnabled = false;

// ========== STORAGE MODE STATE ==========
bool storageModeEnabled = false;

// ========== NETWORK BRIDGE MODE STATE ==========
bool netbridgeModeEnabled = false;

// ========== NETWORK BRIDGE INTERNALS ==========
struct netif* g_ap_netif = NULL;
netif_input_fn g_original_input = NULL;

// Function declarations for network bridge
extern "C" struct netif* eagle_lwip_getif(uint8_t index);

// ========== SLIP CONSTANTS ==========
#define SLIP_END      0xC0u
#define SLIP_ESC      0xDBu
#define SLIP_ESC_END  0xDCu
#define SLIP_ESC_ESC  0xDDu
#define SLIP_MAX_FRAME 1514u

// SLIP receive state (persistent across processSerialData() calls)
static uint8_t  slip_buf[SLIP_MAX_FRAME];
static uint16_t slip_len      = 0;
static bool     slip_in_frame = false;
static bool     slip_escape   = false;

// ========== IMAGE VIEWER STATE (from webimage_viewer MVP) ==========
const uint8_t MARKER_START_IMG[] = {0xFF, 0xD8, 0xFF, 0xAA};  // Modified JPEG SOI
const uint8_t MARKER_END_IMG[] = {0xFF, 0xD9, 0xFF, 0xBB};    // Modified JPEG EOI
#define MARKER_SIZE_IMG 4

uint8_t markerBuffer[MARKER_SIZE_IMG];
int markerPos = 0;
bool inImageTransfer = false;
unsigned long imageBytesReceived = 0;

// WebSocket image buffer for chunked binary sending
#define WS_IMAGE_CHUNK_SIZE 1024
uint8_t wsImageBuffer[WS_IMAGE_CHUNK_SIZE];
int wsImageBufferPos = 0;

// ========== PHANTOMSCRIPT STATE ==========
enum ScriptState {
    SCRIPT_IDLE = 0,
    SCRIPT_RUNNING = 1,
    SCRIPT_PAUSED = 2,
    SCRIPT_STOPPED = 3
};

ScriptState scriptState = SCRIPT_IDLE;
int scriptCurrentLine = 0;
int scriptTotalLines = 0;
PSScriptSettings scriptSettings;  // Global settings for PhantomScript
bool scriptExecutionRequested = false;  // Flag to trigger async execution

File replayFile;
uint32_t replayLastTimestamp = 0;
uint32_t replayNextEventTime = 0;
int replayEventIndex = 0;
char replayNextHIDCommand[128];
bool replayWaitingForNextEvent = false;
unsigned long lastReplayProgressUpdate = 0;

// ========== BUFFERS ==========
String outputBuffer = "";
String serialMonitorBuffer = "";  // Separate buffer for serial monitor data
unsigned long lastWsSendTime = 0;
String lineBuffer = "";

// PowerShell response frame buffers (global for console open/close clearing)
String responseFrameBuffer = "";
bool inResponseFrame = false;
bool serialConsoleOpen = false;  // Track if serial console is open

// Maximum output buffer size (prevent memory exhaustion)
const size_t MAX_OUTPUT_BUFFER_SIZE = 4096;
const size_t MAX_SERIAL_MONITOR_BUFFER_SIZE = 2048;

// ========== BASE64 ENCODING FOR SERIAL MONITOR ==========
// Lightweight base64 encoding function
String base64_encode(const char* input, size_t length) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String encoded = "";

    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];

        while (i++ < 3)
            encoded += '=';
    }

    return encoded;
}

// Lightweight base64 decoding function
String base64_decode(const String& input) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String decoded = "";

    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded += char((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return decoded;
}

// ========== PHANTOMSCRIPT FUNCTIONS ==========

// Load script settings from LittleFS
bool loadScriptSettings() {
    Serial.println("[LOAD] === LOADING SETTINGS FROM FILE ===");
    if (!LittleFS.exists(SCRIPT_SETTINGS_FILE)) {
        Serial.println("[LOAD] No file exists - using defaults");
        scriptSettings.resetToDefaults();
        Serial.printf("[LOAD] Default defaultInterKeystrokeDelay: %d\n", scriptSettings.defaultInterKeystrokeDelay);
        return true;
    }

    File file = LittleFS.open(SCRIPT_SETTINGS_FILE, "r");
    if (!file) {
        Serial.println("[LOAD] ERROR: Failed to open file");
        scriptSettings.resetToDefaults();
        return false;
    }

    size_t fileSize = file.size();
    Serial.printf("[LOAD] File size: %u bytes (expected %u)\n", fileSize, sizeof(PSScriptSettings));

    if (fileSize != sizeof(PSScriptSettings)) {
        file.close();
        Serial.println("[LOAD] ERROR: Wrong file size - using defaults");
        scriptSettings.resetToDefaults();
        return false;
    }

    size_t bytesRead = file.read((uint8_t*)&scriptSettings, sizeof(PSScriptSettings));
    file.close();

    if (bytesRead != sizeof(PSScriptSettings)) {
        Serial.printf("[LOAD] ERROR: Read failed - read %u bytes\n", bytesRead);
        scriptSettings.resetToDefaults();
        return false;
    }

    Serial.printf("[LOAD] Loaded defaultInterKeystrokeDelay from file: %d\n", scriptSettings.defaultInterKeystrokeDelay);

    // Validate loaded settings
    if (scriptSettings.maxVariables < 1 || scriptSettings.maxVariables > 500 ||
        scriptSettings.maxConstants < 1 || scriptSettings.maxConstants > 500 ||
        scriptSettings.maxFunctions < 1 || scriptSettings.maxFunctions > 100 ||
        scriptSettings.maxLoopIterations < 1 || scriptSettings.maxLoopIterations > 1000000 ||
        scriptSettings.maxCallDepth < 1 || scriptSettings.maxCallDepth > 50 ||
        scriptSettings.maxLineLength < 10 || scriptSettings.maxLineLength > 2048 ||
        scriptSettings.maxStringLength < 10 || scriptSettings.maxStringLength > 4096 ||
        scriptSettings.maxScriptLines < 10 || scriptSettings.maxScriptLines > 10000 ||
        scriptSettings.maxExecutionTime < 1000 || scriptSettings.maxExecutionTime > 3600000 ||
        scriptSettings.defaultInterKeystrokeDelay < 0 || scriptSettings.defaultInterKeystrokeDelay > 1000) {
        Serial.println("[LOAD] ERROR: Invalid values - using defaults");
        scriptSettings.resetToDefaults();
        return false;
    }

    Serial.println("[LOAD] SUCCESS - Settings loaded and validated");
    return true;
}

// Save script settings to LittleFS
bool saveScriptSettings() {
    Serial.println("[SAVE] === SAVING SETTINGS TO FILE ===");
    Serial.printf("[SAVE] BEFORE SAVE - defaultInterKeystrokeDelay in memory: %d\n", scriptSettings.defaultInterKeystrokeDelay);

    File file = LittleFS.open(SCRIPT_SETTINGS_FILE, "w");
    if (!file) {
        Serial.println("[SAVE] ERROR: Failed to open file");
        return false;
    }

    size_t bytesWritten = file.write((uint8_t*)&scriptSettings, sizeof(PSScriptSettings));
    if (bytesWritten != sizeof(PSScriptSettings)) {
        Serial.printf("[SAVE] ERROR: Write failed - wrote %u of %u bytes\n", bytesWritten, sizeof(PSScriptSettings));
        file.close();
        return false;
    }

    file.flush();
    file.close();

    // VERIFY: Read back immediately to confirm
    file = LittleFS.open(SCRIPT_SETTINGS_FILE, "r");
    if (file) {
        PSScriptSettings verify;
        file.read((uint8_t*)&verify, sizeof(PSScriptSettings));
        file.close();
        Serial.printf("[SAVE] VERIFY READ - defaultInterKeystrokeDelay in file: %d\n", verify.defaultInterKeystrokeDelay);
        if (verify.defaultInterKeystrokeDelay != scriptSettings.defaultInterKeystrokeDelay) {
            Serial.println("[SAVE] ERROR: Verification failed - file has different value!");
            return false;
        }
    }

    Serial.println("[SAVE] SUCCESS - File written and verified");
    return true;
}

// Get script settings as JSON string
String getScriptSettingsJSON() {
    Serial.printf("[GET-JSON] Reading from memory - defaultInterKeystrokeDelay: %d\n", scriptSettings.defaultInterKeystrokeDelay);
    String json = "{";
    json += "\"maxVariables\":" + String(scriptSettings.maxVariables) + ",";
    json += "\"maxConstants\":" + String(scriptSettings.maxConstants) + ",";
    json += "\"maxFunctions\":" + String(scriptSettings.maxFunctions) + ",";
    json += "\"maxLoopIterations\":" + String(scriptSettings.maxLoopIterations) + ",";
    json += "\"maxCallDepth\":" + String(scriptSettings.maxCallDepth) + ",";
    json += "\"maxLineLength\":" + String(scriptSettings.maxLineLength) + ",";
    json += "\"maxStringLength\":" + String(scriptSettings.maxStringLength) + ",";
    json += "\"maxScriptLines\":" + String(scriptSettings.maxScriptLines) + ",";
    json += "\"maxExecutionTime\":" + String(scriptSettings.maxExecutionTime) + ",";
    json += "\"defaultInterKeystrokeDelay\":" + String(scriptSettings.defaultInterKeystrokeDelay);
    json += "}";
    return json;
}

// HID callback for PhantomScript to send commands to RP2040
void sendHIDToRP2040(const char* hidCommand) {
    Serial.print(hidCommand);
}

// Get script info
String getScriptInfo() {
    if (!LittleFS.exists(SCRIPT_FILE)) {
        return "NONE:0";
    }

    File f = LittleFS.open(SCRIPT_FILE, "r");
    if (!f) {
        return "ERROR:0";
    }

    int lineCount = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        lineCount++;
    }
    f.close();

    String stateStr;
    switch (scriptState) {
        case SCRIPT_IDLE: stateStr = "IDLE"; break;
        case SCRIPT_RUNNING: stateStr = "RUNNING"; break;
        case SCRIPT_PAUSED: stateStr = "PAUSED"; break;
        case SCRIPT_STOPPED: stateStr = "STOPPED"; break;
        default: stateStr = "UNKNOWN"; break;
    }

    return stateStr + ":" + String(lineCount);
}

// Execute script (non-blocking - runs asynchronously)
void executeScriptAsync() {
    if (!LittleFS.exists(SCRIPT_FILE)) {
        ws.textAll((char*)"SCRIPT_ERROR:No script uploaded");
        scriptState = SCRIPT_IDLE;
        scriptExecutionRequested = false;
        return;
    }

    // Ensure script has current settings applied
    phantomScript.setSettings(scriptSettings);

    scriptState = SCRIPT_RUNNING;
    scriptTotalLines = phantomScript.getLineCount();
    String startMsg = "SCRIPT_STARTED:" + String(scriptTotalLines);
    ws.textAll(startMsg);

    // Allow WebSocket to send the message before blocking execution
    yield();
    delay(100);  // Give more time for WebSocket message to be sent

    // Now execute the script (this will block, but we've already sent status)
    bool success = phantomScript.execute(sendHIDToRP2040);

    // Small delay before sending completion message
    delay(50);
    yield();

    if (!success) {
        String error = phantomScript.getError();
        String errorMsg = "SCRIPT_ERROR:" + error;
        ws.textAll(errorMsg);
        scriptState = SCRIPT_IDLE;  // Reset to IDLE on error too
    } else {
        String completeMsg = "SCRIPT_COMPLETED";
        ws.textAll(completeMsg);
        scriptState = SCRIPT_IDLE;
    }

    // Ensure completion message is sent before clearing flag
    delay(50);
    yield();

    scriptExecutionRequested = false;
}

// Delete script file (legacy - kept for compatibility)
bool deleteScript() {
    if (LittleFS.exists(SCRIPT_FILE)) {
        LittleFS.remove(SCRIPT_FILE);
        scriptState = SCRIPT_IDLE;
        return true;
    }
    return false;
}

// ========== SIMPLE SCRIPT MANAGEMENT ==========
// Slot functionality removed for simplicity
// Scripts now only exist in /script.txt (single script system)


// ========== FACTORY RESET ==========

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, LOW);   // LED ON (active LOW)
        delay(delayMs);
        digitalWrite(LED_PIN, HIGH);  // LED OFF
        delay(delayMs);
    }
}

// Returns: 0 = cancelled, 1 = settings reset, 2 = full reset
int checkFactoryReset() {
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED OFF initially

    // Check if factory reset pin is held LOW
    if (digitalRead(FACTORY_RESET_PIN) == HIGH) {
        return 0;  // Pin not pressed, normal boot
    }

    // Immediate LED indication - 3 fast blinks to show detection
    blinkLED(3, 100);

    Serial.println("\n========================================");
    Serial.println("  FACTORY RESET PIN DETECTED");
    Serial.println("  Hold 5s: Settings reset");
    Serial.println("  Hold 10s: Full firmware restore");
    Serial.println("========================================");

    unsigned long startTime = millis();
    bool ledState = false;
    bool settingsResetReached = false;
    bool fullResetReached = false;
    unsigned long lastBlinkTime = millis();

    // Wait and monitor pin state
    while (true) {
        unsigned long elapsed = millis() - startTime;

        // Check if pin released
        if (digitalRead(FACTORY_RESET_PIN) == HIGH) {
            digitalWrite(LED_PIN, HIGH);  // LED OFF

            if (fullResetReached) {
                Serial.println("\n>>> FULL RESET TRIGGERED (10s reached, pin released)");
                return 2;  // Full reset
            } else if (settingsResetReached) {
                Serial.println("\n>>> SETTINGS RESET TRIGGERED (5s reached, pin released)");
                return 1;  // Settings reset only
            } else {
                Serial.println("Factory reset cancelled (released before 5s)");
                return 0;  // Cancelled
            }
        }

        // 5 second milestone - settings reset
        if (elapsed >= SETTINGS_RESET_TIME && !settingsResetReached) {
            settingsResetReached = true;
            Serial.println("\n>>> 5 SECONDS - SETTINGS RESET ARMED");
            Serial.println("    Release now for settings reset only");
            Serial.println("    Hold 5 more seconds for full firmware restore...");
            blinkLED(3, 100);  // 3 fast blinks
            lastBlinkTime = millis();
        }

        // 10 second milestone - full reset
        if (elapsed >= FULL_RESET_TIME && !fullResetReached) {
            fullResetReached = true;
            Serial.println("\n>>> 10 SECONDS - FULL RESET ARMED");
            Serial.println("    Release now for full firmware restore");
            blinkLED(5, 100);  // 5 fast blinks
            lastBlinkTime = millis();
        }

        // Slow blink while waiting (but not right after milestone blinks)
        if (millis() - lastBlinkTime > 500) {
            if (millis() % 500 < 10) {  // Blink every 500ms
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState ? LOW : HIGH);
            }
        }

        yield();
    }
}

void performSettingsReset() {
    Serial.println("\n========================================");
    Serial.println("  PERFORMING SETTINGS RESET");
    Serial.println("========================================");

    // Initialize LittleFS if not already done
    if (!LittleFS.begin()) {
        Serial.println("ERROR: Failed to mount LittleFS");
        blinkLED(10, 50);  // Error indicator
        delay(2000);
        ESP.restart();
        return;
    }

    // Delete AP configuration
    if (LittleFS.exists(AP_CONFIG_FILE)) {
        LittleFS.remove(AP_CONFIG_FILE);
        Serial.println("Deleted AP config");
    }

    // Delete recordings
    if (LittleFS.exists(RECORDING_FILE)) {
        LittleFS.remove(RECORDING_FILE);
        Serial.println("Deleted recordings");
    }

    // Delete OTA staging files (keep factory backup)
    if (LittleFS.exists(OTA_FIRMWARE_FILE)) {
        LittleFS.remove(OTA_FIRMWARE_FILE);
        Serial.println("Deleted OTA staging file");
    }

    if (LittleFS.exists(OTA_FLAG_FILE)) {
        LittleFS.remove(OTA_FLAG_FILE);
        Serial.println("Deleted OTA flag");
    }

    // Delete PhantomScript settings file to reset script settings to defaults
    if (LittleFS.exists(SCRIPT_SETTINGS_FILE)) {
        if (LittleFS.remove(SCRIPT_SETTINGS_FILE)) {
            Serial.println("Deleted PhantomScript settings");
        } else {
            Serial.println("WARNING: Failed to delete PhantomScript settings file");
            // Non-fatal error - continue with reset
        }
    }

    // Delete PhantomScript file to clear any uploaded scripts
    if (LittleFS.exists(SCRIPT_FILE)) {
        if (LittleFS.remove(SCRIPT_FILE)) {
            Serial.println("Deleted PhantomScript file");
        } else {
            Serial.println("WARNING: Failed to delete PhantomScript file");
            // Non-fatal error - continue with reset
        }
    }

    Serial.println("========================================");
    Serial.println("  SETTINGS RESET COMPLETE");
    Serial.println("  Rebooting with default credentials...");
    Serial.println("  (PhantomScript and AP settings reset)");
    Serial.println("  (Firmware version unchanged)");
    Serial.println("========================================");

    // 3 slow blinks to indicate settings reset
    blinkLED(3, 300);

    delay(1000);
    ESP.restart();
}

void performFullReset() {
    Serial.println("\n========================================");
    Serial.println("  PERFORMING FULL FACTORY RESET");
    Serial.println("========================================");

    // Initialize LittleFS if not already done
    if (!LittleFS.begin()) {
        Serial.println("ERROR: Failed to mount LittleFS");
        blinkLED(10, 50);  // Error indicator
        delay(2000);
        ESP.restart();
        return;
    }

    // First, perform settings reset (delete configs)
    if (LittleFS.exists(AP_CONFIG_FILE)) {
        LittleFS.remove(AP_CONFIG_FILE);
        Serial.println("Deleted AP config");
    }

    if (LittleFS.exists(RECORDING_FILE)) {
        LittleFS.remove(RECORDING_FILE);
        Serial.println("Deleted recordings");
    }

    // Delete PhantomScript settings file to reset script settings to defaults
    if (LittleFS.exists(SCRIPT_SETTINGS_FILE)) {
        if (LittleFS.remove(SCRIPT_SETTINGS_FILE)) {
            Serial.println("Deleted PhantomScript settings");
        } else {
            Serial.println("WARNING: Failed to delete PhantomScript settings file");
            // Non-fatal error - continue with reset
        }
    }

    // Delete PhantomScript file to clear any uploaded scripts
    if (LittleFS.exists(SCRIPT_FILE)) {
        if (LittleFS.remove(SCRIPT_FILE)) {
            Serial.println("Deleted PhantomScript file");
        } else {
            Serial.println("WARNING: Failed to delete PhantomScript file");
            // Non-fatal error - continue with reset
        }
    }

    // Check if factory firmware backup exists
    if (!LittleFS.exists(FACTORY_FIRMWARE_FILE)) {
        Serial.println("\n  WARNING: No factory firmware backup found!");
        Serial.println("    Performing settings reset only...");
        Serial.println("    To enable full reset, save factory firmware via web UI");

        // Clean up OTA files
        if (LittleFS.exists(OTA_FIRMWARE_FILE)) {
            LittleFS.remove(OTA_FIRMWARE_FILE);
        }
        if (LittleFS.exists(OTA_FLAG_FILE)) {
            LittleFS.remove(OTA_FLAG_FILE);
        }

        Serial.println("========================================");
        Serial.println("  SETTINGS RESET COMPLETE");
        Serial.println("  (AP, recordings, and PhantomScript reset)");
        Serial.println("  (No factory firmware to restore)");
        Serial.println("========================================");

        blinkLED(3, 300);
        delay(1000);
        ESP.restart();
        return;
    }

    // Verify factory firmware file
    File factoryFile = LittleFS.open(FACTORY_FIRMWARE_FILE, "r");
    if (!factoryFile) {
        Serial.println("ERROR: Cannot open factory firmware file");
        blinkLED(10, 50);
        delay(2000);
        ESP.restart();
        return;
    }

    size_t factorySize = factoryFile.size();
    Serial.printf("Factory firmware found: %u bytes\n", factorySize);

    // Sanity check file size
    if (factorySize < 100000 || factorySize > 1048576) {
        Serial.printf("ERROR: Invalid factory firmware size (%u bytes)\n", factorySize);
        Serial.println("       Expected 100KB - 1MB");
        factoryFile.close();
        blinkLED(10, 50);
        delay(2000);
        ESP.restart();
        return;
    }

    factoryFile.close();

    // Copy factory firmware to OTA staging location
    Serial.println("Copying factory firmware to staging area...");

    // Delete old staging file if exists
    if (LittleFS.exists(OTA_FIRMWARE_FILE)) {
        LittleFS.remove(OTA_FIRMWARE_FILE);
    }

    // Copy factory firmware to staging
    File source = LittleFS.open(FACTORY_FIRMWARE_FILE, "r");
    File dest = LittleFS.open(OTA_FIRMWARE_FILE, "w");

    if (!source || !dest) {
        Serial.println("ERROR: Failed to open files for copying");
        if (source) source.close();
        if (dest) dest.close();
        blinkLED(10, 50);
        delay(2000);
        ESP.restart();
        return;
    }

    // Copy in chunks
    uint8_t buffer[512];
    size_t bytesRead;
    size_t totalCopied = 0;

    while ((bytesRead = source.read(buffer, sizeof(buffer))) > 0) {
        size_t bytesWritten = dest.write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            Serial.println("ERROR: Write failed during copy");
            source.close();
            dest.close();
            blinkLED(10, 50);
            delay(2000);
            ESP.restart();
            return;
        }
        totalCopied += bytesWritten;
        yield();
    }

    source.close();
    dest.close();

    Serial.printf("Copied %u bytes to staging area\n", totalCopied);

    // Verify copy
    if (totalCopied != factorySize) {
        Serial.printf("ERROR: Copy size mismatch (expected %u, got %u)\n", factorySize, totalCopied);
        blinkLED(10, 50);
        delay(2000);
        ESP.restart();
        return;
    }

    // Create OTA flag to trigger recovery mode
    File flagFile = LittleFS.open(OTA_FLAG_FILE, "w");
    if (flagFile) {
        flagFile.print("1");
        flagFile.close();
        Serial.println("OTA flag set");
    } else {
        Serial.println("ERROR: Failed to create OTA flag");
        blinkLED(10, 50);
        delay(2000);
        ESP.restart();
        return;
    }

    Serial.println("========================================");
    Serial.println("  FULL FACTORY RESET COMPLETE");
    Serial.println("  All settings and scripts deleted");
    Serial.println("  Rebooting to recovery mode...");
    Serial.println("  Factory firmware will be restored");
    Serial.println("========================================");

    // 3 slow blinks to indicate full reset
    blinkLED(3, 300);

    delay(1000);
    ESP.restart();
}

// ========== AP CONFIGURATION ==========

void loadAPConfig() {
    if (!LittleFS.exists(AP_CONFIG_FILE)) {
        Serial.println("No saved AP config, using defaults");
        return;
    }

    File configFile = LittleFS.open(AP_CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("Failed to open AP config file");
        return;
    }

    String content = configFile.readString();
    configFile.close();

    int commaPos = content.indexOf(',');
    if (commaPos > 0) {
        String ssid = content.substring(0, commaPos);
        String password = content.substring(commaPos + 1);

        ssid.trim();
        password.trim();

        if (ssid.length() > 0 && ssid.length() <= 32 &&
            password.length() >= 8 && password.length() <= 63) {
            strncpy(AP_SSID, ssid.c_str(), sizeof(AP_SSID) - 1);
            AP_SSID[sizeof(AP_SSID) - 1] = '\0';
            strncpy(AP_PASSWORD, password.c_str(), sizeof(AP_PASSWORD) - 1);
            AP_PASSWORD[sizeof(AP_PASSWORD) - 1] = '\0';
            Serial.println("Loaded AP config from file");
            Serial.printf("SSID: %s\n", AP_SSID);
        } else {
            Serial.println("Invalid AP config, using defaults");
        }
    }
}

bool saveAPConfig(const String& ssid, const String& password) {
    File configFile = LittleFS.open(AP_CONFIG_FILE, "w");
    if (!configFile) {
        Serial.println("Failed to create AP config file");
        return false;
    }

    configFile.print(ssid);
    configFile.print(",");
    configFile.print(password);
    configFile.close();

    Serial.println("AP config saved successfully");
    return true;
}

// ========== OTA RECOVERY MODE ==========

void performOTARecovery() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n========================================");
    Serial.println("  OTA RECOVERY MODE");
    Serial.println("  Flashing firmware...");
    Serial.println("========================================");

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("ERROR: Failed to mount LittleFS");
        delay(2000);
        ESP.restart();
        return;
    }

    // Remove flag file
    if (LittleFS.exists(OTA_FLAG_FILE)) {
        LittleFS.remove(OTA_FLAG_FILE);
    }

    // Check if firmware file exists
    if (!LittleFS.exists(OTA_FIRMWARE_FILE)) {
        Serial.println("ERROR: Firmware file not found");
        delay(2000);
        ESP.restart();
        return;
    }

    File firmware = LittleFS.open(OTA_FIRMWARE_FILE, "r");
    if (!firmware) {
        Serial.println("ERROR: Failed to open firmware file");
        delay(2000);
        ESP.restart();
        return;
    }

    size_t fileSize = firmware.size();
    Serial.printf("Firmware size: %u bytes\n", fileSize);
    Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());

    // Validate size
    if (fileSize < 100000) {
        Serial.printf("ERROR: Firmware too small (%u bytes) - likely corrupt\n", fileSize);
        firmware.close();
        LittleFS.remove(OTA_FIRMWARE_FILE);
        delay(2000);
        ESP.restart();
        return;
    }

    // Begin update
    if (!Update.begin(fileSize, U_FLASH)) {
        Serial.println("ERROR: Update.begin() failed");
        Update.printError(Serial);
        firmware.close();
        delay(2000);
        ESP.restart();
        return;
    }

    Serial.println("Writing firmware to flash...");

    // Write firmware in chunks
    uint8_t buffer[512];
    size_t written = 0;
    unsigned long lastPrint = 0;

    while (firmware.available()) {
        size_t toRead = min(sizeof(buffer), (size_t)firmware.available());
        size_t bytesRead = firmware.read(buffer, toRead);

        if (bytesRead > 0) {
            ESP.wdtDisable();
            size_t bytesWritten = Update.write(buffer, bytesRead);
            ESP.wdtEnable(5000);

            if (bytesWritten != bytesRead) {
                Serial.printf("ERROR: Write failed - wrote %u of %u bytes\n", bytesWritten, bytesRead);
                firmware.close();
                delay(2000);
                ESP.restart();
                return;
            }

            written += bytesWritten;

            // Print progress every 500ms
            if (millis() - lastPrint > 500) {
                Serial.printf("Progress: %u / %u bytes (%.1f%%)\n",
                    written, fileSize, (written * 100.0) / fileSize);
                lastPrint = millis();
            }
        }

        yield();
    }

    firmware.close();

    Serial.printf("Total written: %u bytes\n", written);
    Serial.println("Finalizing update...");

    // Finalize update
    if (Update.end(true)) {
        if (Update.isFinished()) {
            Serial.println("========================================");
            Serial.println("UPDATE SUCCESSFUL!");
            Serial.println("========================================");

            // Delete firmware file
            LittleFS.remove(OTA_FIRMWARE_FILE);

            Serial.println("Rebooting to new firmware in 3 seconds...");
            delay(3000);
            ESP.restart();
        } else {
            Serial.println("ERROR: Update not finished");
            Update.printError(Serial);
            delay(2000);
            ESP.restart();
        }
    } else {
        Serial.println("ERROR: Update.end() failed");
        Update.printError(Serial);
        delay(2000);
        ESP.restart();
    }
}


// ========== RECORDING MANAGEMENT ==========

void initRecording() {
    // LittleFS is already initialized in setup()
    Serial.println("Recording system ready");
}

bool startRecording() {
    if (recState != REC_IDLE) return false;
    if (LittleFS.exists(RECORDING_FILE)) LittleFS.remove(RECORDING_FILE);
    recordingFile = LittleFS.open(RECORDING_FILE, "w");
    if (!recordingFile) return false;
    recState = REC_RECORDING;
    recordingStartTime = millis();
    recordingEntryCount = 0;
    Serial.println("Recording started");
    return true;
}

bool stopRecording() {
    if (recState != REC_RECORDING) return false;
    if (recordingFile) recordingFile.close();
    recState = REC_STOPPED;
    Serial.printf("Recording stopped. %d events captured\n", recordingEntryCount);
    return true;
}

bool recordHIDEvent(const String& rawHID) {
    if (recState != REC_RECORDING) return false;
    if (recordingEntryCount >= MAX_RECORDING_ENTRIES) {
        stopRecording();
        return false;
    }
    if (!recordingFile) return false;
    uint32_t timestamp = millis() - recordingStartTime;
    recordingFile.printf("%lu,%s\n", timestamp, rawHID.c_str());
    recordingEntryCount++;
    return true;
}

bool clearRecording() {
    if (recState == REC_RECORDING) stopRecording();
    if (LittleFS.exists(RECORDING_FILE)) LittleFS.remove(RECORDING_FILE);
    recState = REC_IDLE;
    recordingEntryCount = 0;
    Serial.println("Recording cleared");
    return true;
}

bool startReplayRecording() {
    if (recState != REC_STOPPED) return false;
    if (!LittleFS.exists(RECORDING_FILE)) return false;
    replayFile = LittleFS.open(RECORDING_FILE, "r");
    if (!replayFile) return false;
    recState = REC_REPLAYING;
    replayLastTimestamp = 0;
    replayNextEventTime = millis();
    replayEventIndex = 0;
    replayWaitingForNextEvent = false;
    lastReplayProgressUpdate = 0;
    memset(replayNextHIDCommand, 0, sizeof(replayNextHIDCommand));
    Serial.printf("Replaying recording with %d events...\n", recordingEntryCount);
    ws.textAll((char*)"REPLAY_START");
    return true;
}

void stopReplay() {
    if (replayFile) replayFile.close();
    recState = REC_STOPPED;
    replayWaitingForNextEvent = false;
    ws.textAll((char*)"REPLAY_COMPLETE");
    Serial.println("Replay complete");
}

void processReplay() {
    if (recState != REC_REPLAYING) return;
    unsigned long currentTime = millis();
    if (replayWaitingForNextEvent) {
        if (currentTime >= replayNextEventTime) {
            Serial.print(replayNextHIDCommand);
            Serial.print("\r\n");
            if (currentTime - lastReplayProgressUpdate >= 500) {
                char progress[64];
                snprintf(progress, sizeof(progress), "REPLAY_PROGRESS:%d/%d", replayEventIndex, recordingEntryCount);
                ws.textAll(progress);
                lastReplayProgressUpdate = currentTime;
            }
            replayWaitingForNextEvent = false;
        } else {
            return;
        }
    }
    if (replayFile.available()) {
        char lineBuffer[128];
        size_t bytesRead = replayFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[bytesRead] = '\0';
        char* lineStart = lineBuffer;
        while (*lineStart == ' ' || *lineStart == '\r' || *lineStart == '\n') lineStart++;
        if (strlen(lineStart) > 0) {
            char* commaPos = strchr(lineStart, ',');
            if (commaPos != NULL) {
                *commaPos = '\0';
                uint32_t timestamp = strtoul(lineStart, NULL, 10);
                char* hidCommand = commaPos + 1;
                strncpy(replayNextHIDCommand, hidCommand, sizeof(replayNextHIDCommand) - 1);
                replayNextHIDCommand[sizeof(replayNextHIDCommand) - 1] = '\0';
                if (replayEventIndex > 0) {
                    uint32_t delay_ms = timestamp - replayLastTimestamp;
                    replayNextEventTime = currentTime + delay_ms;
                } else {
                    replayNextEventTime = currentTime;
                }
                replayLastTimestamp = timestamp;
                replayEventIndex++;
                replayWaitingForNextEvent = true;
            }
        }
    } else {
        stopReplay();
    }
}

String getRecordingInfo() {
    char info[128];
    size_t fileSize = 0;
    if (LittleFS.exists(RECORDING_FILE)) {
        File f = LittleFS.open(RECORDING_FILE, "r");
        if (f) {
            fileSize = f.size();
            f.close();
        }
    }
    snprintf(info, sizeof(info), "{\"state\":%d,\"count\":%d,\"size\":%u}",
             recState, recordingEntryCount, fileSize);
    return String(info);
}

// ========== HID PARSING (UNCHANGED) ==========

bool parseHIDLine(const String& line, uint8_t* reportId, uint8_t* data, uint16_t* dataLen) {
    if (!line.startsWith("HID:")) return false;
    int colonPos1 = line.indexOf(':', 4);
    if (colonPos1 < 0) return false;
    int colonPos2 = line.indexOf(':', colonPos1 + 1);
    if (colonPos2 < 0) return false;
    String reportIdStr = line.substring(4, colonPos1);
    *reportId = strtol(reportIdStr.c_str(), nullptr, 16);
    String lenStr = line.substring(colonPos1 + 1, colonPos2);
    *dataLen = strtol(lenStr.c_str(), nullptr, 16);
    if (*dataLen > 64) return false;
    String hexData = line.substring(colonPos2 + 1);
    hexData.trim();
    for (uint16_t i = 0; i < *dataLen; i++) {
        if (i * 2 + 1 >= hexData.length()) break;
        char byteStr[3] = {hexData[i * 2], hexData[i * 2 + 1], '\0'};
        data[i] = strtol(byteStr, nullptr, 16);
    }
    return true;
}

String processHIDMessage(const String& line) {
    uint8_t reportId;
    uint8_t data[64];
    uint16_t dataLen;
    if (!parseHIDLine(line, &reportId, data, &dataLen)) return "";
    if (recState == REC_RECORDING) recordHIDEvent(line);
    String parsed = hidParser.parseHIDReport(reportId, data, dataLen);
    if (parsed.length() > 0) return parsed + "\n";
    return "";
}

String processATResponse(const String& line) {
    String trimmed = line;
    trimmed.trim();
    if (trimmed.startsWith("+INFO:")) {
        int ccPos = trimmed.indexOf("CC=");
        if (ccPos > 0) {
            String ccStr = trimmed.substring(ccPos + 3);
            uint8_t countryCode = strtol(ccStr.c_str(), nullptr, 16);
            hidParser.setLayout(countryCode);
        }
    }
    else if (trimmed.startsWith("+SERIAL:")) {
        // Parse: +SERIAL:ON or +SERIAL:OFF
        Serial.printf("[RP2040-RECV] Received SERIAL response from RP2040: %s\n", trimmed.c_str());

        if (trimmed.indexOf("ON") > 0) {
            Serial.printf("[STATE-UPDATE] Updating serialModeEnabled: %s -> true\n", serialModeEnabled ? "true" : "false");
            serialModeEnabled = true;
            Serial.println("[STATE-UPDATE] Serial CDC mode enabled");
        } else {
            Serial.printf("[STATE-UPDATE] Updating serialModeEnabled: %s -> false\n", serialModeEnabled ? "true" : "false");
            serialModeEnabled = false;
            Serial.println("[STATE-UPDATE] Serial CDC mode disabled");
        }
        Serial.println("[WS-BROADCAST] Broadcasting SERIAL state to all WebSocket clients...");
    }
    else if (trimmed.startsWith("+HYBRID:")) {
        // Parse: +HYBRID:ON or +HYBRID:OFF
        Serial.printf("[RP2040-RECV] Received HYBRID response from RP2040: %s\n", trimmed.c_str());

        if (trimmed.indexOf("ON") > 0) {
            Serial.printf("[STATE-UPDATE] Updating hybridModeEnabled: %s -> true\n", hybridModeEnabled ? "true" : "false");
            hybridModeEnabled = true;
            Serial.println("[STATE-UPDATE] Hybrid mode enabled");
        } else {
            Serial.printf("[STATE-UPDATE] Updating hybridModeEnabled: %s -> false\n", hybridModeEnabled ? "true" : "false");
            hybridModeEnabled = false;
            Serial.println("[STATE-UPDATE] Hybrid mode disabled");
        }
        Serial.println("[WS-BROADCAST] Broadcasting HYBRID state to all WebSocket clients...");
    }
    else if (trimmed.startsWith("+STORAGE:")) {
        // Parse: +STORAGE:ON or +STORAGE:OFF
        Serial.printf("[RP2040-RECV] Received STORAGE response from RP2040: %s\n", trimmed.c_str());

        if (trimmed.indexOf("ON") > 0) {
            Serial.printf("[STATE-UPDATE] Updating storageModeEnabled: %s -> true\n", storageModeEnabled ? "true" : "false");
            storageModeEnabled = true;
            Serial.println("[STATE-UPDATE] Storage mode enabled");
        } else {
            Serial.printf("[STATE-UPDATE] Updating storageModeEnabled: %s -> false\n", storageModeEnabled ? "true" : "false");
            storageModeEnabled = false;
            Serial.println("[STATE-UPDATE] Storage mode disabled");
        }
        Serial.println("[WS-BROADCAST] Broadcasting STORAGE state to all WebSocket clients...");
    }
    else if (trimmed.startsWith("+NETBRIDGE:")) {
        // Parse: +NETBRIDGE:ON or +NETBRIDGE:OFF
        Serial.printf("[RP2040-RECV] Received NETBRIDGE response from RP2040: %s\n", trimmed.c_str());

        bool newState = (trimmed.indexOf("ON") > 0);
        bool oldState = netbridgeModeEnabled;

        if (newState != oldState) {
            Serial.printf("[STATE-UPDATE] Updating netbridgeModeEnabled: %s -> %s\n",
                         oldState ? "true" : "false", newState ? "true" : "false");

            netbridgeModeEnabled = newState;

            if (newState) {
                Serial.println("[STATE-UPDATE] Network bridge mode enabled - setting up hooks");
                setup_network_bridge();
            } else {
                Serial.println("[STATE-UPDATE] Network bridge mode disabled - removing hooks");
                teardown_network_bridge();
            }
        }
        Serial.println("[WS-BROADCAST] Broadcasting NETBRIDGE state to all WebSocket clients...");
    }
    return "";
}

// ========== WEBSOCKET HELPERS ==========

// Safe text send to client with validity checks
bool safeSendToClient(AsyncWebSocketClient *client, const char* message) {
    if (!client || client->status() != WS_CONNECTED) {
        return false;
    }

    // Check memory before sending
    if (ESP.getFreeHeap() < 4096) {
        Serial.println("WARNING: Low memory, skipping WebSocket send");
        return false;
    }

    client->text(message);
    yield();  // Allow other tasks to run
    return true;
}

// Safe text send with formatted string (prevents String concatenation)
bool safeSendToClientF(AsyncWebSocketClient *client, const char* format, ...) {
    if (!client || client->status() != WS_CONNECTED) {
        return false;
    }

    // Check memory before sending
    if (ESP.getFreeHeap() < 4096) {
        Serial.println("WARNING: Low memory, skipping WebSocket send");
        return false;
    }

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    client->text(buffer);
    yield();  // Allow other tasks to run
    return true;
}

// ========== WEBSOCKET EVENT HANDLER ==========

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
    static char buffer[512];  // Static buffer for messages

    if (type == WS_EVT_CONNECT) {
        // Send recording info on connect
        if (client) {
            String recInfo = getRecordingInfo();
            snprintf(buffer, sizeof(buffer), "REC_INFO:%s", recInfo.c_str());
            client->text(buffer);
        }
        // Device info is now requested from webpage.h after WebSocket connects
        // (removed automatic AT+INFO send to avoid duplicate display)
    } else if (type == WS_EVT_DISCONNECT) {
        // Nothing to do
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len) {
            if (info->opcode == WS_TEXT) {
                // Copy message to buffer
                if (len >= sizeof(buffer)) len = sizeof(buffer) - 1;
                memcpy(buffer, data, len);
                buffer[len] = '\0';

                // Trim whitespace
                char* message = buffer;
                while (*message == ' ' || *message == '\t' || *message == '\r' || *message == '\n') message++;
                char* end = message + strlen(message) - 1;
                while (end > message && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
                *(end + 1) = '\0';

                // Handle commands (using strcmp for string comparison)
                if (strcmp(message, "REC_START") == 0) {
                    if (startRecording()) client->text("REC_STARTED");
                    else client->text("REC_ERROR:Failed to start recording");
                } else if (strcmp(message, "REC_STOP") == 0) {
                    static char recStopBuf[32];
                    if (stopRecording()) {
                        snprintf(recStopBuf, sizeof(recStopBuf), "REC_STOPPED:%d", recordingEntryCount);
                        client->text(recStopBuf);
                    } else {
                        client->text("REC_ERROR:Not recording");
                    }
                } else if (strcmp(message, "REC_CLEAR") == 0) {
                    if (clearRecording()) client->text("REC_CLEARED");
                } else if (strcmp(message, "REC_REPLAY") == 0) {
                    if (!startReplayRecording()) client->text("REC_ERROR:Failed to start replay");
                } else if (strcmp(message, "REC_INFO") == 0) {
                    static char recInfoBuf[64];
                    String recInfo = getRecordingInfo();
                    snprintf(recInfoBuf, sizeof(recInfoBuf), "REC_INFO:%s", recInfo.c_str());
                    client->text(recInfoBuf);
                } else if (strncmp(message, "HYBRID_SET:", 11) == 0) {
                    const char* state = message + 11;
                    Serial.printf("[WS-RECV] Received HYBRID_SET command from UI: %s\n", message);

                    static char atBuf[32];
                    snprintf(atBuf, sizeof(atBuf), "AT+HYBRID=%s\r\n", state);

                    Serial.printf("[RP2040-SEND] Sending AT command to RP2040: %s", atBuf);
                    Serial.print(atBuf);
                    Serial.flush();
                    Serial.println("[RP2040-SEND] AT command flushed to RP2040. Waiting for response...");

                } else if (strncmp(message, "SERIAL_SET:", 11) == 0) {
                    const char* state = message + 11;
                    Serial.printf("[WS-RECV] Received SERIAL_SET command from UI: %s\n", message);

                    static char atBuf[32];
                    snprintf(atBuf, sizeof(atBuf), "AT+SERIAL=%s\r\n", state);

                    Serial.printf("[RP2040-SEND] Sending AT command to RP2040: %s", atBuf);
                    Serial.print(atBuf);
                    Serial.flush();
                    Serial.println("[RP2040-SEND] AT command flushed to RP2040. Waiting for response...");

                } else if (strncmp(message, "STORAGE_SET:", 12) == 0) {
                    const char* state = message + 12;
                    Serial.printf("[WS-RECV] Received STORAGE_SET command from UI: %s\n", message);

                    static char atBuf[32];
                    snprintf(atBuf, sizeof(atBuf), "AT+STORAGE=%s\r\n", state);

                    Serial.printf("[RP2040-SEND] Sending AT command to RP2040: %s", atBuf);
                    Serial.print(atBuf);
                    Serial.flush();
                    Serial.println("[RP2040-SEND] AT command flushed to RP2040. Waiting for response...");

                } else if (strncmp(message, "NETBRIDGE_SET:", 14) == 0) {
                    const char* state = message + 14;
                    Serial.printf("[WS-RECV] Received NETBRIDGE_SET command from UI: %s\n", message);

                    static char atBuf[32];
                    snprintf(atBuf, sizeof(atBuf), "AT+NETBRIDGE=%s\r\n", state);

                    Serial.printf("[RP2040-SEND] Sending AT command to RP2040: %s", atBuf);
                    Serial.print(atBuf);
                    Serial.flush();
                    Serial.println("[RP2040-SEND] AT command flushed to RP2040. Waiting for response...");

                } else if (strncmp(message, "VKEY_PRESS:", 11) == 0) {
                    // Virtual keyboard key press: VKEY_PRESS:0xXX
                    const char* keycodeStr = message + 11;
                    uint8_t keycode = (uint8_t)strtol(keycodeStr, NULL, 16);
                    if (vkeyboard_pressKey(keycode)) {
                        String hidCmd = vkeyboard_getHIDString();
                        Serial.print(hidCmd);
                        Serial.print("\r\n");
                        Serial.flush();
                    }
                } else if (strncmp(message, "VKEY_RELEASE:", 13) == 0) {
                    // Virtual keyboard key release: VKEY_RELEASE:0xXX
                    const char* keycodeStr = message + 13;
                    uint8_t keycode = (uint8_t)strtol(keycodeStr, NULL, 16);
                    if (vkeyboard_releaseKey(keycode)) {
                        String hidCmd = vkeyboard_getHIDString();
                        Serial.print(hidCmd);
                        Serial.print("\r\n");
                        Serial.flush();
                    }
                } else if (strcmp(message, "VKEY_CLEAR") == 0) {
                    // Clear all virtual keyboard keys
                    vkeyboard_releaseAll();
                    String hidCmd = vkeyboard_getHIDString();
                    Serial.print(hidCmd);
                    Serial.print("\r\n");
                    Serial.flush();
                } else if (strncmp(message, "VMOUSE_MOVE:", 12) == 0) {
                    // Virtual mouse move: VMOUSE_MOVE:X:Y (e.g., VMOUSE_MOVE:5:-3)
                    const char* params = message + 12;
                    const char* colon = strchr(params, ':');
                    if (colon) {
                        int8_t x = (int8_t)atoi(params);
                        int8_t y = (int8_t)atoi(colon + 1);
                        // Generate mouse HID report: HID:02:0004:BBXXYYWW (buttons, x, y, wheel)
                        static char hidCmd[32];
                        snprintf(hidCmd, sizeof(hidCmd), "HID:02:0004:%02X%02X%02X00\r\n",
                                0x00, (uint8_t)x, (uint8_t)y);
                        Serial.print(hidCmd);
                        Serial.flush();
                    }
                } else if (strncmp(message, "VMOUSE_BUTTON:", 14) == 0) {
                    // Virtual mouse button: VMOUSE_BUTTON:N:STATE (e.g., VMOUSE_BUTTON:1:1 = left button down)
                    const char* params = message + 14;
                    const char* colon = strchr(params, ':');
                    if (colon) {
                        uint8_t buttonNum = (uint8_t)atoi(params);
                        uint8_t state = (uint8_t)atoi(colon + 1);
                        static uint8_t mouseButtons = 0;
                        if (state) {
                            mouseButtons |= (1 << (buttonNum - 1));
                        } else {
                            mouseButtons &= ~(1 << (buttonNum - 1));
                        }
                        // Generate mouse HID report with button state
                        static char hidCmd[32];
                        snprintf(hidCmd, sizeof(hidCmd), "HID:02:0004:%02X00000000\r\n", mouseButtons);
                        Serial.print(hidCmd);
                        Serial.flush();
                    }
                } else if (strncmp(message, "VMOUSE_WHEEL:", 13) == 0) {
                    // Virtual mouse wheel: VMOUSE_WHEEL:N (e.g., VMOUSE_WHEEL:1 = scroll up)
                    const char* wheelStr = message + 13;
                    int8_t wheel = (int8_t)atoi(wheelStr);
                    // Generate mouse HID report with wheel movement
                    static char hidCmd[32];
                    snprintf(hidCmd, sizeof(hidCmd), "HID:02:0004:000000%02X\r\n", (uint8_t)wheel);
                    Serial.print(hidCmd);
                    Serial.flush();
                } else if (strncmp(message, "SCRIPT_LOAD_STRING:", 19) == 0) {
                    // Load PhantomScript from string and execute
                    const char* scriptContent = message + 19;

                    Serial.println("Loading script from string...");

                    // Reset PhantomScript engine
                    phantomScript = PhantomScript();
                    phantomScript.setSettings(scriptSettings);

                    // Load script from string (loadFromString accepts const char*)
                    if (phantomScript.loadFromString(String(scriptContent))) {
                        scriptState = SCRIPT_IDLE;
                        ws.textAll((char*)"SCRIPT_LOADED");
                        Serial.println("Script loaded successfully");

                        // Auto-execute the script
                        scriptExecutionRequested = true;
                        Serial.println("Script execution requested");
                    } else {
                        static char errBuf[128];
                        String error = phantomScript.getError();
                        snprintf(errBuf, sizeof(errBuf), "SCRIPT_ERROR:%s", error.c_str());
                        ws.textAll(errBuf);
                        scriptState = SCRIPT_IDLE;
                        Serial.print("Script load failed: ");
                        Serial.println(error);
                    }
                } else if (strcmp(message, "SCRIPT_SETTINGS_GET") == 0) {
                    // Return current script settings as JSON
                    static char settingsBuf[256];
                    String settingsJSON = getScriptSettingsJSON();
                    snprintf(settingsBuf, sizeof(settingsBuf), "SCRIPT_SETTINGS:%s", settingsJSON.c_str());
                    client->text(settingsBuf);
                } else if (strncmp(message, "SCRIPT_SETTINGS_SET:", 20) == 0) {
                    // Parse and save new script settings - SIMPLE VERSION
                    const char* jsonStr = message + 20;
                    Serial.printf("[WS] === RECEIVED SETTINGS TO SAVE ===\n");
                    Serial.printf("[WS] JSON: %s\n", jsonStr);

                    // Parse JSON manually - update scriptSettings directly
                    const char* ptr;
                    if ((ptr = strstr(jsonStr, "\"maxVariables\":")) != NULL) {
                        scriptSettings.maxVariables = atoi(ptr + 15);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxConstants\":")) != NULL) {
                        scriptSettings.maxConstants = atoi(ptr + 15);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxFunctions\":")) != NULL) {
                        scriptSettings.maxFunctions = atoi(ptr + 15);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxLoopIterations\":")) != NULL) {
                        scriptSettings.maxLoopIterations = atoi(ptr + 20);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxCallDepth\":")) != NULL) {
                        scriptSettings.maxCallDepth = atoi(ptr + 15);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxLineLength\":")) != NULL) {
                        scriptSettings.maxLineLength = atoi(ptr + 16);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxStringLength\":")) != NULL) {
                        scriptSettings.maxStringLength = atoi(ptr + 18);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxScriptLines\":")) != NULL) {
                        scriptSettings.maxScriptLines = atoi(ptr + 17);
                    }
                    if ((ptr = strstr(jsonStr, "\"maxExecutionTime\":")) != NULL) {
                        scriptSettings.maxExecutionTime = atoi(ptr + 19);
                    }
                    if ((ptr = strstr(jsonStr, "\"defaultInterKeystrokeDelay\":")) != NULL) {
                        scriptSettings.defaultInterKeystrokeDelay = atoi(ptr + 30);
                        Serial.printf("[WS] PARSED defaultInterKeystrokeDelay: %d\n", scriptSettings.defaultInterKeystrokeDelay);
                    }

                    Serial.printf("[WS] BEFORE SAVE - scriptSettings.defaultInterKeystrokeDelay = %d\n", scriptSettings.defaultInterKeystrokeDelay);

                    // Just save to file - that's it!
                    if (saveScriptSettings()) {
                        Serial.printf("[WS] AFTER SAVE - scriptSettings.defaultInterKeystrokeDelay = %d\n", scriptSettings.defaultInterKeystrokeDelay);
                        client->text("SCRIPT_SETTINGS_OK");
                    } else {
                        client->text("SCRIPT_SETTINGS_ERROR:Failed to save");
                    }
                } else if (strcmp(message, "SCRIPT_SETTINGS_RESET") == 0) {
                    // Reset script settings to defaults - SIMPLE VERSION
                    scriptSettings.resetToDefaults();

                    if (saveScriptSettings()) {
                        client->text("SCRIPT_SETTINGS_RESET_OK");
                    } else {
                        client->text("SCRIPT_SETTINGS_ERROR:Failed to save defaults");
                    }
                } else if (strcmp(message, "PS_CONSOLE_OPEN") == 0) {
                    // PowerShell console opened - clear any pending response buffers
                    serialConsoleOpen = true;
                    responseFrameBuffer = "";
                    inResponseFrame = false;
                    Serial.println("[PS] Console opened - buffers cleared");
                    // No response needed
                } else if (strcmp(message, "PS_CONSOLE_CLOSE") == 0) {
                    // PowerShell console closed - clear any pending response buffers
                    serialConsoleOpen = false;
                    responseFrameBuffer = "";
                    inResponseFrame = false;
                    Serial.println("[PS] Console closed - buffers cleared");
                    // No response needed
                } else if (strncmp(message, "SERIAL_SEND:", 12) == 0) {
                    // Serial monitor send command - encode and forward to RP2040
                    const char* data_to_send = message + 12;  // Skip 'SERIAL_SEND:' prefix
                    size_t data_len = strlen(data_to_send);

                    // DEBUG: Log when ESP8266 receives and processes the command
                    Serial.printf("[ESP] SERIAL_SEND received: '%s' (len=%d)\n", data_to_send, data_len);

                    // Base64 encode the data for reliable transmission
                    String encoded = base64_encode(data_to_send, data_len);

                    // DEBUG: Log before sending to RP2040
                    Serial.printf("[ESP] Sending to RP2040: SERIAL:CMD_START:%s:CMD_END\n", encoded.c_str());

                    // Send to RP2040 with SERIAL: prefix and CMD markers
                    // Format: SERIAL:CMD_START:<base64_data>:CMD_END\r\n
                    Serial.print("SERIAL:CMD_START:");
                    Serial.print(encoded);
                    Serial.print(":CMD_END\r\n");
                    Serial.flush();

                    // DEBUG: Confirm flush completed
                    Serial.println("[ESP] Serial.flush() completed");

                    // No response needed - data is forwarded transparently
                } else {
                    // Forward everything else to Serial (AT commands, etc.)

                    // DEBUG: Log AT command reception and forwarding
                    if (strncmp(message, "AT+", 3) == 0) {
                        Serial.printf("\n[ESP8266-DEBUG] Received AT command from WebSocket: %s\n", message);
                        Serial.printf("[ESP8266-DEBUG] Forwarding to RP2040 via UART...\n");
                    }

                    Serial.write(data, len);
                    // Check if message ends with \r\n
                    bool needsNewline = true;
                    if (strlen(message) >= 2) {
                        needsNewline = !(message[strlen(message)-2] == '\r' && message[strlen(message)-1] == '\n');
                    }
                    if (needsNewline) Serial.print("\r\n");
                    Serial.flush();

                    // DEBUG: Confirm sent to UART
                    if (strncmp(message, "AT+", 3) == 0) {
                        Serial.printf("[ESP8266-DEBUG] AT command sent to RP2040 and flushed\n\n");
                    }
                }
            }
        }
    }
}

// ========== IMAGE WEBSOCKET EVENT HANDLER ==========

void onWsImageEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                    void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("\n========================================\n");
        Serial.printf("[wsImage] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        Serial.printf("[wsImage] Total clients: %u\n", wsImage.count());
        Serial.printf("[wsImage] Free heap: %u bytes\n", ESP.getFreeHeap());
        Serial.printf("========================================\n");
        // Reset image transfer state on new connection
        inImageTransfer = false;
        imageBytesReceived = 0;
        wsImageBufferPos = 0;
        markerPos = 0;
        memset(markerBuffer, 0, MARKER_SIZE_IMG);
        // Clear text buffers too
        outputBuffer = "";
        lineBuffer = "";
        serialMonitorBuffer = "";
        Serial.println("[wsImage] Switched to BINARY MODE - ready for image data");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("\n========================================\n");
        Serial.printf("[wsImage] Client #%u disconnected\n", client->id());
        Serial.printf("[wsImage] Remaining clients: %u\n", wsImage.count());
        Serial.printf("========================================\n");
        // Reset state when last client disconnects
        if (wsImage.count() == 0) {
            inImageTransfer = false;
            imageBytesReceived = 0;
            wsImageBufferPos = 0;
            markerPos = 0;
            memset(markerBuffer, 0, MARKER_SIZE_IMG);
            outputBuffer = "";
            lineBuffer = "";
            serialMonitorBuffer = "";
            Serial.println("[wsImage] Modal closed - switched to TEXT MODE");
        }
    }
    // No text message handling needed - binary only
}

// ========== CAPTIVE PORTAL ==========

class CaptiveRequestHandler : public AsyncWebHandler {
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}
    bool canHandle(AsyncWebServerRequest *request) { return true; }
    void handleRequest(AsyncWebServerRequest *request) {
        String host = request->host();
        if (host == apIP.toString()) {
            request->send_P(200, "text/html", index_html);
        } else {
            request->redirect("http://" + apIP.toString());
        }
    }
};

// ========== SERIAL DATA PROCESSING (UNCHANGED) ==========

// ========== IMAGE VIEWER FUNCTIONS (from webimage_viewer MVP) ==========

void flushImageWSBuffer() {
    if (wsImageBufferPos > 0) {
        // Check if any clients are connected to image WebSocket
        if (wsImage.count() == 0) {
            Serial.println("[WARN] No clients connected to image WebSocket, clearing buffer");
            wsImageBufferPos = 0;
            markerPos = 0;
            inImageTransfer = false;
            return;
        }

        // Memory guard: Check free heap before sending
        if (ESP.getFreeHeap() < 8192) {
            Serial.printf("[WARN] Low memory (%u bytes), skipping binary send\n", ESP.getFreeHeap());
            wsImageBufferPos = 0;
            return;
        }

        // Check buffer size is valid
        if (wsImageBufferPos > WS_IMAGE_CHUNK_SIZE) {
            Serial.printf("[ERROR] Buffer overflow detected: %d > %d\n", wsImageBufferPos, WS_IMAGE_CHUNK_SIZE);
            wsImageBufferPos = 0;
            markerPos = 0;
            inImageTransfer = false;
            return;
        }

        // CRITICAL: Feed watchdog before potentially blocking WebSocket operation
        ESP.wdtFeed();

        // Send to image WebSocket
        wsImage.binaryAll(wsImageBuffer, wsImageBufferPos);

        yield();  // Allow other tasks to run
        delay(5);  // Rate limiting: prevents overwhelming WebSocket (~200KB/sec max)

        wsImageBufferPos = 0;
    }
}

void addToImageWSBuffer(const uint8_t* data, int len) {
    for (int i = 0; i < len; i++) {
        wsImageBuffer[wsImageBufferPos++] = data[i];

        if (wsImageBufferPos >= WS_IMAGE_CHUNK_SIZE) {
            flushImageWSBuffer();
        }
    }
}

bool checkImageMarker(const uint8_t* marker) {
    for (int i = 0; i < MARKER_SIZE_IMG; i++) {
        if (markerBuffer[i] != marker[i]) return false;
    }
    return true;
}

void processSerialByteForImage(uint8_t byte) {
    // CRITICAL: Always process bytes to detect END marker, even if modal closed
    // This prevents getting stuck in binary mode when user closes modal mid-transfer

    // Add byte to marker buffer
    markerBuffer[markerPos] = byte;
    markerPos++;

    // Check if we have enough bytes for a marker
    if (markerPos == MARKER_SIZE_IMG) {
        // Check for START marker
        if (checkImageMarker(MARKER_START_IMG)) {
            // CRITICAL: Clear all text buffers before binary transfer
            outputBuffer = "";
            lineBuffer = "";

            // CRITICAL: Flush any pending data before starting new image
            flushImageWSBuffer();

            // Clear marker buffer completely
            memset(markerBuffer, 0, MARKER_SIZE_IMG);
            markerPos = 0;

            // Only send to WebSocket if modal is open
            if (wsImage.count() > 0) {
                wsImage.binaryAll(MARKER_START_IMG, MARKER_SIZE_IMG);
                yield();
                Serial.println("Image transfer started");
            } else {
                Serial.println("Image transfer started (discarding - modal closed)");
            }

            inImageTransfer = true;
            imageBytesReceived = 0;
            return;
        }

        // Check for END marker
        if (checkImageMarker(MARKER_END_IMG)) {
            // CRITICAL: Flush remaining image data
            flushImageWSBuffer();

            // Clear marker buffer
            memset(markerBuffer, 0, MARKER_SIZE_IMG);
            markerPos = 0;

            // Only send to WebSocket if modal is open
            if (wsImage.count() > 0) {
                wsImage.binaryAll(MARKER_END_IMG, MARKER_SIZE_IMG);
                yield();
                Serial.printf("Image transfer complete: %lu bytes\n", imageBytesReceived);
            } else {
                Serial.printf("Image transfer complete (discarded): %lu bytes\n", imageBytesReceived);
            }

            inImageTransfer = false;
            return;
        }

        // Not a marker, buffer oldest byte only if in transfer AND modal is open
        if (inImageTransfer && wsImage.count() > 0) {
            wsImageBuffer[wsImageBufferPos++] = markerBuffer[0];
            imageBytesReceived++;

            if (wsImageBufferPos >= WS_IMAGE_CHUNK_SIZE) {
                flushImageWSBuffer();
                yield();
            }
        }
        // If modal closed mid-transfer, silently discard bytes (no panic)

        // Shift buffer left
        for (int i = 0; i < MARKER_SIZE_IMG - 1; i++) {
            markerBuffer[i] = markerBuffer[i + 1];
        }
        markerPos = MARKER_SIZE_IMG - 1;
    }
}

void processSerialData() {
    static unsigned long lastMemCheck = 0;
    int bytesProcessed = 0;
    const int MAX_BYTES_PER_LOOP = 256;  // Reduced from 512 to allow more frequent main loop iteration

    while (Serial.available() > 0 && bytesProcessed < MAX_BYTES_PER_LOOP) {
        uint8_t byte = Serial.read();
        bytesProcessed++;

        // Periodic memory check (every 100 bytes)
        if (bytesProcessed % 100 == 0) {
            if (ESP.getFreeHeap() < 4096) {
                Serial.printf("[WARN] Low memory: %u bytes free\n", ESP.getFreeHeap());
                // Clear buffers to free memory
                outputBuffer = "";
                lineBuffer = "";
            }
        }

        // CRITICAL: Two-way valve - Binary mode OR Text mode, never both
        // When screenshot modal is open (wsImage has clients), ONLY process binary
        if (wsImage.count() > 0) {
            // Screenshot modal is open - BINARY MODE ONLY
            processSerialByteForImage(byte);

            // Yield MORE frequently during image transfer to prevent WDT reset
            if (bytesProcessed % 32 == 0) {
                yield();
                ESP.wdtFeed();  // Explicit watchdog feed
            }
            continue;  // Skip ALL text processing below
        }

        // CRITICAL: During image transfer, ONLY process binary data
        // Skip all text processing to prevent memory conflicts
        if (inImageTransfer) {
            processSerialByteForImage(byte);
            yield();  // Prevent watchdog timeout during large binary transfers
            ESP.wdtFeed();  // Explicit watchdog feed
            continue;  // Skip all text processing below
        }

        // ---------- SLIP frame handler (runs before text mode) ----------
        // 0xC0 and 0xDB are non-ASCII; they don't appear in AT/HID text commands.
        if (byte == SLIP_END) {
            if (slip_in_frame && slip_len > 0) {
                // Dispatch complete SLIP frame → inject into WiFi AP
                inject_packet_from_uart(slip_buf, slip_len);
            }
            slip_in_frame = true;
            slip_len      = 0;
            slip_escape   = false;
            continue;
        }
        if (slip_in_frame) {
            if (byte == SLIP_ESC) {
                slip_escape = true;
            } else {
                uint8_t decoded;
                if (slip_escape) {
                    decoded     = (byte == SLIP_ESC_END) ? SLIP_END :
                                  (byte == SLIP_ESC_ESC) ? SLIP_ESC : byte;
                    slip_escape = false;
                } else {
                    decoded = byte;
                }
                if (slip_len < SLIP_MAX_FRAME) {
                    slip_buf[slip_len++] = decoded;
                } else {
                    // Frame too large — reset
                    slip_in_frame = false;
                    slip_len      = 0;
                }
            }
            continue;
        }

        // Screenshot modal is closed - TEXT MODE ONLY
        // Process as text for HID/AT commands
        char c = (char)byte;

        // Priority 1: Check for PowerShell response frame markers (RESP_START: ... :RESP_END)
        // NEW PROTOCOL: Send markers separately, stream base64 chunks for real-time
        if (inResponseFrame) {
            // Already collecting response frame data
            responseFrameBuffer += c;

            // Check for :RESP_END marker
            if (responseFrameBuffer.endsWith(":RESP_END")) {
                // Only send if console is open
                if (serialConsoleOpen) {
                    // Extract base64 before end marker
                    String base64Data = responseFrameBuffer.substring(0, responseFrameBuffer.length() - 9);

                    // Send remaining base64 chunk if any
                    if (base64Data.length() > 0) {
                        ws.textAll(base64Data);
                    }

                    // Send end marker as separate message
                    ws.textAll(":RESP_END");
                } else {
                    Serial.println("[PS] Discarding response frame - console not open");
                }

                // Reset frame state
                responseFrameBuffer = "";
                inResponseFrame = false;
            }
            // Stream base64 chunks immediately (512 byte chunks for real-time)
            else if (responseFrameBuffer.length() >= 512) {
                // Only send if console is open
                if (serialConsoleOpen) {
                    // Send chunk to WebSocket immediately
                    ws.textAll(responseFrameBuffer);
                }
                responseFrameBuffer = "";
            }

            // Safety: prevent buffer overflow for malformed frames
            if (responseFrameBuffer.length() > 8192) {
                // Frame too large or malformed, reset
                responseFrameBuffer = "";
                inResponseFrame = false;
                if (serialConsoleOpen) {
                    ws.textAll(":RESP_END");  // Terminate frame
                }
            }

            continue;  // Skip normal line processing while collecting frame
        }

        // Priority 2: Check for line ending (process complete line)
        if (c == '\r' || c == '\n') {
            if (lineBuffer.length() > 0) {
                // Process normal lines (HID/AT/NET commands)
                String processedLine = "";

                if (lineBuffer.startsWith("HID:")) {
                    processedLine = processHIDMessage(lineBuffer);
                } else if (lineBuffer.startsWith("OK") ||
                         lineBuffer.startsWith("+INFO:") ||
                         lineBuffer.startsWith("+TYPE:") ||
                         lineBuffer.startsWith("+LAYOUT:") ||
                         lineBuffer.startsWith("+HYBRID:") ||
                         lineBuffer.startsWith("+SERIAL:") ||
                         lineBuffer.startsWith("+STORAGE:") ||
                         lineBuffer.startsWith("+NETBRIDGE:") ||
                         lineBuffer.startsWith("ERROR:")) {

                    // DEBUG: Log AT response reception
                    if (lineBuffer.startsWith("+INFO:")) {
                        Serial.printf("\n[ESP8266-DEBUG] Received AT response from RP2040: %s\n", lineBuffer.c_str());
                        Serial.printf("[ESP8266-DEBUG] Adding to outputBuffer for WebSocket transmission\n");
                    }

                    processATResponse(lineBuffer);
                    processedLine = "> " + lineBuffer + "\n";

                    // DEBUG: Log buffer length
                    if (lineBuffer.startsWith("+INFO:")) {
                        Serial.printf("[ESP8266-DEBUG] outputBuffer will be: %d bytes\n\n", outputBuffer.length() + processedLine.length());
                    }
                } else if (lineBuffer.startsWith("WebSocket")) {
                    processedLine = "";
                } else {
                    processedLine = lineBuffer + "\n";
                }
                if (processedLine.length() > 0) outputBuffer += processedLine;

                // Serial monitor buffer is ONLY populated by PowerShell response frames
                // (handled in RESP_START:...:RESP_END frame decoder above at line ~1772)
                // Do NOT add AT responses, HID events, or other data here
                // This keeps serial monitor clean for PowerShell interaction only

                lineBuffer = "";
            }
        } else {
            // Priority 3: Regular character - add to line buffer
            lineBuffer += c;

            // Check if lineBuffer now contains RESP_START: marker
            // PowerShell sends frames without line endings, so check continuously
            if (lineBuffer.indexOf("RESP_START:") >= 0) {
                int startIdx = lineBuffer.indexOf("RESP_START:");

                // Only send start marker if console is open
                if (serialConsoleOpen) {
                    // Send start marker as separate WebSocket message
                    ws.textAll("RESP_START:");
                }

                inResponseFrame = true;
                responseFrameBuffer = lineBuffer.substring(startIdx + 11);  // Skip "RESP_START:"
                lineBuffer = "";
                continue;  // Continue collecting frame data
            }

            if (lineBuffer.length() > 512) lineBuffer = "";
        }

        yield();  // Allow other tasks to run (like WebSocket)
    }
}

void sendBufferedData() {
    // CRITICAL: Don't send text data when in binary mode (modal open) or during transfer
    if (inImageTransfer || wsImage.count() > 0) {
        // Clear buffers to prevent memory buildup while in binary mode
        if (outputBuffer.length() > 1024) {
            outputBuffer = "";
        }
        if (serialMonitorBuffer.length() > 1024) {
            serialMonitorBuffer = "";
        }
        return;
    }

    unsigned long currentTime = millis();

    // Force immediate send for AT command responses (contains "> +")
    // This prevents INFO responses from being delayed by throttling
    bool hasATResponse = outputBuffer.indexOf("> +") >= 0;
    bool hasINFOResponse = outputBuffer.indexOf("> +INFO:") >= 0;

    if (outputBuffer.length() > 0 &&
        (hasATResponse ||  // Immediate send for AT responses
         currentTime - lastWsSendTime >= WS_SEND_INTERVAL ||
         outputBuffer.length() >= WS_BUFFER_SIZE)) {

        // DEBUG: Log when sending INFO response
        if (hasINFOResponse) {
            Serial.printf("[ESP8266-DEBUG] sendBufferedData() - Sending INFO response to WebSocket\n");
            Serial.printf("[ESP8266-DEBUG] Buffer size: %d bytes\n", outputBuffer.length());
            Serial.printf("[ESP8266-DEBUG] hasATResponse: %s\n", hasATResponse ? "true" : "false");
            Serial.printf("[ESP8266-DEBUG] Time since last send: %lu ms\n", currentTime - lastWsSendTime);
            Serial.printf("[ESP8266-DEBUG] WebSocket clients connected: %d\n\n", ws.count());
        }

        lastWsSendTime = currentTime;
        ws.textAll(outputBuffer);
        outputBuffer = "";
    }

    // Serial monitor now uses real-time streaming (sent immediately via ws.textAll)
    // No buffering needed - chunks sent as they arrive
}

// ========== OTA UPLOAD HANDLER (TWO-STAGE: SAVE TO LITTLEFS) ==========

void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    static size_t totalWritten = 0;
    static size_t expectedSize = 0;
    static unsigned long lastProgress = 0;

    if (!index) {
        // Start of upload - save to LittleFS
        Serial.println("\n========================================");
        Serial.printf("OTA Upload Start: %s\n", filename.c_str());
        Serial.printf("Content-Length: %u bytes\n", request->contentLength());
        Serial.printf("Current version: %s\n", FIRMWARE_VERSION);
        Serial.println("Saving to LittleFS...");
        Serial.println("========================================");

        expectedSize = request->contentLength();

        // Delete old firmware file if exists
        if (LittleFS.exists(OTA_FIRMWARE_FILE)) {
            LittleFS.remove(OTA_FIRMWARE_FILE);
        }

        // Open file for writing
        uploadFile = LittleFS.open(OTA_FIRMWARE_FILE, "w");
        if (!uploadFile) {
            Serial.println("ERROR: Failed to open firmware file for writing");
            ws.textAll((char*)"OTA_ERROR:Failed to open file");
            return;
        }

        totalWritten = 0;
        lastProgress = 0;
        ws.textAll((char*)"OTA_PROGRESS:0");
    }

    // Write chunk to LittleFS
    if (len) {
        if (uploadFile) {
            size_t written = uploadFile.write(data, len);
            if (written != len) {
                Serial.printf("ERROR: Write failed - wrote %u of %u bytes\n", written, len);
                uploadFile.close();
                LittleFS.remove(OTA_FIRMWARE_FILE);
                ws.textAll((char*)"OTA_ERROR:Write failed");
                return;
            }

            totalWritten += written;

            // Send progress update - calculate percentage if we know expected size
            if (expectedSize > 0) {
                int percent = (totalWritten * 100) / expectedSize;
                unsigned long now = millis();
                if (now - lastProgress >= 500) {
                    char progress[32];
                    snprintf(progress, sizeof(progress), "OTA_PROGRESS:%d", percent);
                    ws.textAll(progress);
                    Serial.printf("Upload progress: %u / %u bytes (%d%%)\n", totalWritten, expectedSize, percent);
                    lastProgress = now;
                }
            }
        }
    }

    // End of upload
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }

        Serial.println("========================================");
        Serial.printf("Upload complete: %u bytes written (expected %u)\n", totalWritten, expectedSize);

        // Verify file exists and has content
        if (!LittleFS.exists(OTA_FIRMWARE_FILE)) {
            Serial.println("ERROR: Firmware file not found after upload");
            ws.textAll((char*)"OTA_ERROR:File verification failed");
            return;
        }

        File verify = LittleFS.open(OTA_FIRMWARE_FILE, "r");
        if (!verify) {
            Serial.println("ERROR: Cannot open firmware file for verification");
            ws.textAll((char*)"OTA_ERROR:Cannot verify file");
            return;
        }

        size_t actualSize = verify.size();
        verify.close();

        Serial.printf("Verified size: %u bytes\n", actualSize);

        if (actualSize < 100000) {
            Serial.printf("ERROR: Firmware too small (%u bytes) - likely corrupt\n", actualSize);
            LittleFS.remove(OTA_FIRMWARE_FILE);
            ws.textAll((char*)"OTA_ERROR:Firmware too small");
            return;
        }

        // Create flag file to trigger recovery mode on next boot
        File flagFile = LittleFS.open(OTA_FLAG_FILE, "w");
        if (flagFile) {
            flagFile.print("1");
            flagFile.close();
            Serial.println("OTA flag set - will enter recovery on reboot");
        } else {
            Serial.println("ERROR: Failed to create OTA flag");
            ws.textAll((char*)"OTA_ERROR:Flag creation failed");
            return;
        }

        Serial.println("Firmware file saved successfully");

        // Send 100% progress and success BEFORE reboot
        ws.textAll((char*)"OTA_PROGRESS:100");
        delay(100);
        ws.textAll((char*)"OTA_REBOOTING");
        delay(500);

        Serial.println("========================================");
        Serial.println("Rebooting to recovery mode NOW...");
        Serial.println("========================================");
        delay(200);

        // Reboot to recovery mode
        ESP.restart();
    }
}

// ========== FACTORY FIRMWARE UPLOAD HANDLER ==========

void handleFactoryFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    static size_t totalWritten = 0;
    static size_t expectedSize = 0;

    if (!index) {
        // Start of upload
        Serial.println("\n========================================");
        Serial.printf("Factory Firmware Upload: %s\n", filename.c_str());
        Serial.printf("Size: %u bytes\n", request->contentLength());
        Serial.println("========================================");

        expectedSize = request->contentLength();

        // Delete old factory firmware if exists
        if (LittleFS.exists(FACTORY_FIRMWARE_FILE)) {
            LittleFS.remove(FACTORY_FIRMWARE_FILE);
        }

        // Open file for writing
        uploadFile = LittleFS.open(FACTORY_FIRMWARE_FILE, "w");
        if (!uploadFile) {
            Serial.println("ERROR: Failed to open factory firmware file for writing");
            return;
        }

        totalWritten = 0;
    }

    // Write chunk
    if (len) {
        if (uploadFile) {
            size_t written = uploadFile.write(data, len);
            if (written != len) {
                Serial.printf("ERROR: Write failed - wrote %u of %u bytes\n", written, len);
                uploadFile.close();
                LittleFS.remove(FACTORY_FIRMWARE_FILE);
                return;
            }
            totalWritten += written;
        }
    }

    // End of upload
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }

        Serial.println("========================================");
        Serial.printf("Factory firmware upload complete: %u bytes\n", totalWritten);

        // Verify
        if (totalWritten < 100000) {
            Serial.printf("ERROR: Firmware too small (%u bytes)\n", totalWritten);
            LittleFS.remove(FACTORY_FIRMWARE_FILE);
            return;
        }

        Serial.println("Factory firmware saved successfully");
        Serial.println("========================================");
    }
}

// ========== SCRIPT UPLOAD HANDLER ==========

void handleScriptUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    static size_t totalWritten = 0;

    if (!index) {
        Serial.println("\n========================================");
        Serial.printf("PhantomScript Upload: %s\n", filename.c_str());
        Serial.printf("Size: %u bytes\n", request->contentLength());
        Serial.println("========================================");

        // Check file size (max 100KB for scripts)
        if (request->contentLength() > 102400) {
            Serial.println("ERROR: Script file too large (max 100KB)");
            ws.textAll((char*)"SCRIPT_ERROR:File too large");
            return;
        }

        // Delete old script if exists
        if (LittleFS.exists(SCRIPT_FILE)) {
            LittleFS.remove(SCRIPT_FILE);
        }

        // Open file for writing
        uploadFile = LittleFS.open(SCRIPT_FILE, "w");
        if (!uploadFile) {
            Serial.println("ERROR: Failed to open script file for writing");
            ws.textAll((char*)"SCRIPT_ERROR:Failed to write");
            return;
        }

        totalWritten = 0;
    }

    // Write chunk
    if (len) {
        if (uploadFile) {
            size_t written = uploadFile.write(data, len);
            if (written != len) {
                Serial.printf("ERROR: Write failed - wrote %u of %u bytes\n", written, len);
                uploadFile.close();
                LittleFS.remove(SCRIPT_FILE);
                ws.textAll((char*)"SCRIPT_ERROR:Write failed");
                return;
            }
            totalWritten += written;
        }
    }

    // End of upload
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }

        Serial.println("========================================");
        Serial.printf("Script upload complete: %u bytes\n", totalWritten);

        // Validate script file
        if (totalWritten < 5) {
            Serial.println("ERROR: Script too small (min 5 bytes)");
            LittleFS.remove(SCRIPT_FILE);
            ws.textAll((char*)"SCRIPT_ERROR:File too small");
            return;
        }

        // Count lines and check for binary content
        File verify = LittleFS.open(SCRIPT_FILE, "r");
        int lines = 0;
        int nullCount = 0;
        int readCount = 0;

        if (verify) {
            while (verify.available()) {
                int c = verify.read();
                if (readCount < 256) {
                    if (c == 0) nullCount++;
                    readCount++;
                }
                if (c == '\n') lines++;
            }
            verify.close();

            if (nullCount > 10) {
                Serial.println("ERROR: Script contains excessive null bytes (binary file?)");
                LittleFS.remove(SCRIPT_FILE);
                ws.textAll((char*)"SCRIPT_ERROR:Invalid file format");
                return;
            }
        }

        Serial.printf("Script saved (%d lines)\n", lines);
        Serial.println("========================================");

        // Just load the script, don't execute
        phantomScript = PhantomScript();  // Reset
        phantomScript.setSettings(scriptSettings);
        if (phantomScript.load(SCRIPT_FILE)) {
            scriptState = SCRIPT_IDLE;
            ws.textAll((char*)"SCRIPT_UPLOAD_SUCCESS");
        } else {
            ws.textAll((char*)"SCRIPT_ERROR:Failed to load script");
        }
    }
}

// ========== NETWORK BRIDGE FUNCTIONS ==========

// Check if pbuf packet should be forwarded to UART (destined for USB side)
bool should_forward_to_uart_from_pbuf(struct pbuf* p) {
    if (!netbridgeModeEnabled) return false;
    if (p->len < 14 + 20) return false;  // Need Ethernet + IP headers

    uint8_t* eth_payload = (uint8_t*)p->payload;

    // Check EtherType for IPv4 (0x0800)
    uint16_t ethertype = (eth_payload[12] << 8) | eth_payload[13];
    if (ethertype != 0x0800) return false;  // Only forward IPv4

    // Extract destination IP from IP header (14 bytes Eth + 16 bytes into IP header)
    uint8_t* ip_header = eth_payload + 14;
    uint32_t dest_ip = ((uint32_t)ip_header[16] << 24) |
                       ((uint32_t)ip_header[17] << 16) |
                       ((uint32_t)ip_header[18] << 8) |
                       ((uint32_t)ip_header[19]);

    // Forward packets destined for USB side (192.168.4.2-10) or RP2040 (192.168.4.100)
    uint32_t subnet_base = (192UL << 24) | (168UL << 16) | (4UL << 8);
    uint32_t usb_range_start = subnet_base | 2;
    uint32_t usb_range_end = subnet_base | 10;
    uint32_t rp2040_ip = subnet_base | 100;

    return ((dest_ip >= usb_range_start && dest_ip <= usb_range_end) ||
            (dest_ip == rp2040_ip));
}

// Forward pbuf to RP2040 via SLIP framing
void forward_pbuf_to_uart(struct pbuf* p) {
    uint16_t total_len = p->tot_len;
    if (total_len == 0 || total_len > SLIP_MAX_FRAME) return;

    if (ESP.getFreeHeap() < 8192) return;  // low-memory guard

    uint8_t* buf = (uint8_t*)malloc(total_len);
    if (!buf) return;

    pbuf_copy_partial(p, buf, total_len, 0);

    Serial.write(SLIP_END);
    for (uint16_t i = 0; i < total_len; i++) {
        if (buf[i] == SLIP_END) {
            Serial.write(SLIP_ESC);
            Serial.write(SLIP_ESC_END);
        } else if (buf[i] == SLIP_ESC) {
            Serial.write(SLIP_ESC);
            Serial.write(SLIP_ESC_ESC);
        } else {
            Serial.write(buf[i]);
        }
    }
    Serial.write(SLIP_END);

    free(buf);
}

// Custom netif input callback for WiFi → lwIP interception
err_t bridge_netif_input(struct pbuf* p, struct netif* inp) {
    if (!netbridgeModeEnabled || g_original_input == NULL) {
        // Bridge disabled or not initialized - use original input
        if (g_original_input) {
            return (*g_original_input)(p, inp);
        }
        return ERR_OK;
    }

    // Check if packet should be forwarded to UART (USB side)
    if (should_forward_to_uart_from_pbuf(p)) {
        // Forward to RP2040 via UART
        forward_pbuf_to_uart(p);
        // Packet consumed - free it
        pbuf_free(p);
        return ERR_OK;
    }

    // Process locally using original input function
    return (*g_original_input)(p, inp);
}

// Inject NET: packet from UART into WiFi network
void inject_packet_from_uart(const uint8_t* packet_data, uint16_t len) {
    if (!netbridgeModeEnabled) return;

    // Validate packet size
    if (len < 14 || len > 1514) {
        Serial.printf("[NET-BRIDGE] Invalid packet size: %d\n", len);
        return;
    }

    // Check memory
    if (ESP.getFreeHeap() < 8192) {
        Serial.println("[NET-BRIDGE] Low memory - dropping inbound packet");
        return;
    }

    // Check if AP netif is ready
    if (g_ap_netif == NULL || !netif_is_up(g_ap_netif)) {
        Serial.println("[NET-BRIDGE] AP netif not ready");
        return;
    }

    // Allocate pbuf for the packet (PBUF_RAW includes Ethernet header)
    struct pbuf* p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p == NULL) {
        Serial.println("[NET-BRIDGE] Failed to allocate pbuf");
        return;
    }

    // Copy packet data into pbuf
    pbuf_take(p, packet_data, len);

    // Inject into WiFi AP network interface
    err_t err = ethernet_input(p, g_ap_netif);

    if (err != ERR_OK) {
        Serial.printf("[NET-BRIDGE] Packet injection failed: %d\n", err);
        pbuf_free(p);
    }
    // Note: Don't free pbuf on success - lwIP takes ownership
}

// Install network bridge hooks on AP interface
void setup_network_bridge() {
    if (!netbridgeModeEnabled) return;

    Serial.println("[NET-BRIDGE] Setting up network bridge...");

    // Small delay to ensure WiFi AP is fully initialized
    delay(500);

    // Get SoftAP network interface (index 1)
    g_ap_netif = eagle_lwip_getif(1);

    if (g_ap_netif == NULL) {
        Serial.println("[NET-BRIDGE] ERROR: Failed to get AP netif!");
        return;
    }

    Serial.printf("[NET-BRIDGE] AP netif found: %c%c%d\n",
                  g_ap_netif->name[0], g_ap_netif->name[1], g_ap_netif->num);
    Serial.printf("[NET-BRIDGE] AP IP: %s\n",
                  IPAddress(g_ap_netif->ip_addr.addr).toString().c_str());

    // Save original input callback
    g_original_input = g_ap_netif->input;

    // Replace with our custom callback for packet interception
    g_ap_netif->input = bridge_netif_input;

    Serial.println("[NET-BRIDGE] Network bridge hooks installed successfully!");
    Serial.println("[NET-BRIDGE] Packet forwarding between WiFi (192.168.4.50+) and USB (192.168.4.2-10) active");
}

// Remove network bridge hooks
void teardown_network_bridge() {
    if (g_ap_netif != NULL && g_original_input != NULL) {
        // Restore original input callback
        g_ap_netif->input = g_original_input;
        g_original_input = NULL;
        Serial.println("[NET-BRIDGE] Network bridge hooks removed");
    }
    g_ap_netif = NULL;
}

// ========== SETUP ==========

void setup() {
    // Configure UART - GPIO1(TX) and GPIO3(RX) are default UART0 pins
    Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
    Serial.begin(BAUD_RATE);
    delay(500);

    // Check for factory reset FIRST (before LittleFS)
    int resetMode = checkFactoryReset();
    if (resetMode == 1) {
        performSettingsReset();
        // Never reaches here - will reboot
        return;
    } else if (resetMode == 2) {
        performFullReset();
        // Never reaches here - will reboot
        return;
    }
    // resetMode == 0: Normal boot, continue

    // Initialize LittleFS first
    if (!LittleFS.begin()) {
        Serial.println("ERROR: Failed to mount LittleFS");
        delay(2000);
        ESP.restart();
        return;
    }

    // Check if we need to enter recovery mode
    if (LittleFS.exists(OTA_FLAG_FILE)) {
        performOTARecovery();
        // Never reaches here - recovery mode will reboot
        return;
    }

    // NORMAL MODE
    Serial.println("\n\n========================================");
    Serial.print("  ESP8266 HID Console v");
    Serial.println(FIRMWARE_VERSION);
    Serial.println("  With Recording & Replay + Two-Stage OTA");
    Serial.println("========================================");

    Serial.println("LittleFS mounted successfully");

    // Load AP configuration from file
    loadAPConfig();

    // Load script settings from file
    loadScriptSettings();
    phantomScript.setSettings(scriptSettings);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    Serial.println("\nAccess Point Started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("IP Address: ");
    Serial.println(apIP);
    Serial.println("\nCaptive Portal Active");
    Serial.println("========================================\n");

    // Setup network bridge if enabled (after WiFi AP is initialized)
    if (netbridgeModeEnabled) {
        setup_network_bridge();
    }
    
    dnsServer.start(DNS_PORT, "*", apIP);

    // Register main WebSocket for text commands (HID, AT, recording)
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Register dedicated image WebSocket for binary streaming
    wsImage.onEvent(onWsImageEvent);
    server.addHandler(&wsImage);
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    
    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", FIRMWARE_VERSION);
    });

    server.on("/ap_ssid", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", AP_SSID);
    });

    server.on("/factory_status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        if (LittleFS.exists(FACTORY_FIRMWARE_FILE)) {
            File f = LittleFS.open(FACTORY_FIRMWARE_FILE, "r");
            if (f) {
                size_t size = f.size();
                f.close();
                json += "\"exists\":true,\"size\":" + String(size) + ",\"version\":\"" + FIRMWARE_VERSION + "\"";
            } else {
                json += "\"exists\":false";
            }
        } else {
            json += "\"exists\":false";
        }
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/factory_delete", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists(FACTORY_FIRMWARE_FILE)) {
            if (LittleFS.remove(FACTORY_FIRMWARE_FILE)) {
                Serial.println("Factory firmware deleted via web UI");
                request->send(200, "text/plain", "OK");
            } else {
                request->send(500, "text/plain", "ERROR");
            }
        } else {
            request->send(404, "text/plain", "NOT_FOUND");
        }
    });
    
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://" + apIP.toString());
    });
    
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://" + apIP.toString());
    });
    
    server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://" + apIP.toString());
    });
    
    server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://" + apIP.toString());
    });
    
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://" + apIP.toString());
    });
    
    // OTA endpoint - saves to LittleFS, sets flag, then reboots to recovery
    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "OK");
        },
        handleOTAUpload
    );

    // Factory firmware upload endpoint
    server.on("/factory_upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "OK");
        },
        handleFactoryFirmwareUpload
    );

    // PhantomScript upload endpoint
    server.on("/script_upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "OK");
        },
        handleScriptUpload
    );

    // Script info endpoint
    server.on("/script_info", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", getScriptInfo());
    });


    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    
    server.begin();
    Serial.println("Web server ready");
    Serial.println("HID parser initialized");
    initRecording();
    vkeyboard_init();
    Serial.println();
}

// ========== MAIN LOOP ==========

void loop() {
    dnsServer.processNextRequest();

    // Cleanup WebSocket clients frequently to ensure events are processed
    static unsigned long lastWsCleanup = 0;
    if (millis() - lastWsCleanup > 100) {  // Reduced from 5000ms to 100ms
        ws.cleanupClients();
        wsImage.cleanupClients();  // Also cleanup image WebSocket clients
        lastWsCleanup = millis();
    }

    // Execute script if requested (non-blocking trigger from WebSocket)
    if (scriptExecutionRequested && scriptState == SCRIPT_IDLE) {
        executeScriptAsync();
        // Script execution is now complete, flag is cleared inside executeScriptAsync
    }

    if (recState != REC_REPLAYING) {
        processSerialData();
    } else {
        outputBuffer = "";
    }

    processReplay();

    if (recState != REC_REPLAYING) {
        sendBufferedData();
    }

    static unsigned long lastRecUpdate = 0;
    if (recState == REC_RECORDING && millis() - lastRecUpdate > 1000) {
        char countMsg[32];
        snprintf(countMsg, sizeof(countMsg), "REC_COUNT:%d", recordingEntryCount);
        ws.textAll(countMsg);
        lastRecUpdate = millis();
    }

    // Periodically flush response buffer for real-time streaming (like shell-over-serial.ino)
    static unsigned long lastResponseFlush = 0;
    if (inResponseFrame && serialConsoleOpen && responseFrameBuffer.length() > 0 &&
        (millis() - lastResponseFlush) > 50) {
        // Send accumulated data even if buffer hasn't reached 512 bytes
        ws.textAll(responseFrameBuffer);
        responseFrameBuffer = "";
        lastResponseFlush = millis();
    }

    yield();
}