#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "pico/mutex.h"
#include "hardware/watchdog.h"

#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "uart_comm.h"
#include "net_lwip.h"

#define RESET_DELAY_MS 200
#define DESCRIPTOR_TIMEOUT_MS 5000
#define WATCHDOG_TIMEOUT_MS 8000
#define MIN_RESET_INTERVAL_MS 1000

// CDC forwarding buffer for serial CDC data
#define CDC_BUFFER_SIZE 4096
static uint8_t cdc_buffer[CDC_BUFFER_SIZE];

// Image transfer markers (from webimage_viewer MVP) - currently unused
// static const uint8_t MARKER_START[] = {0xFF, 0xD8, 0xFF, 0xAA};
// static const uint8_t MARKER_END[] = {0xFF, 0xD9, 0xFF, 0xBB};
// #define MARKER_SIZE 4

extern void core1_main();
extern volatile bool device_connected;
extern volatile bool trigger_reset;
extern volatile bool use_device_info;
extern mutex_t queue_mutex;
extern mutex_t state_mutex;
extern volatile descriptor_type_t current_descriptor_type;
extern volatile device_info_t connected_device_info;

static uint32_t last_reset_time = 0;

// Wrapper function to match callback signature (void return type)
static void net_packet_callback_wrapper(const uint8_t* packet, uint16_t len) {
    // Call the actual forwarding function and discard return value
    (void)net_forward_from_uart(packet, len);
}

int main(void) {
    set_sys_clock_khz(120000, true);
    sleep_ms(10);

    mutex_init(&queue_mutex);
    mutex_init(&state_mutex);

    uart_comm_init();  // Initializes UART and both CDC↔UART queues

    // Register network packet callback for forwarding UART NET: packets to lwIP stack
    uart_set_net_packet_callback(net_packet_callback_wrapper);

    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    // Wait for Core 1 to detect any pre-connected devices before starting USB device stack
    // This prevents enumeration with default descriptors when device is already connected
    // Timeout after 2 seconds to ensure boot even if no device is connected
    uint32_t startup_wait_start = to_ms_since_boot(get_absolute_time());
    bool device_detected_at_startup = false;
    while (to_ms_since_boot(get_absolute_time()) - startup_wait_start < 2000) {
        watchdog_update();

        mutex_enter_blocking(&state_mutex);
        bool device_is_connected = device_connected;
        bool descriptor_set = (current_descriptor_type != DESC_TYPE_DEFAULT || connected_device_info.valid);
        mutex_exit(&state_mutex);

        if (device_is_connected && descriptor_set) {
            device_detected_at_startup = true;
            break;
        }

        sleep_ms(10);
    }

    // If device was detected at startup, enable device info spoofing
    if (device_detected_at_startup) {
        sleep_ms(100);  // Extra delay to ensure descriptor and device info are fully set

        mutex_enter_blocking(&state_mutex);
        if (connected_device_info.valid) {
            use_device_info = true;
        }
        mutex_exit(&state_mutex);
    }
    watchdog_update();

    tud_init(0);

    while (true) {
        tud_task();
        uart_process_received_data();
        uart_drain_tx_queue();        // Drain unified TX queue (HID intercepts + NET packets + AT responses)
        process_cdc_to_uart_queue();  // Process queued CDC→UART data non-blocking (serial mode only)
        process_uart_to_cdc_queue();  // Process queued UART→CDC data non-blocking

        // Process network stack timers when network mode is enabled
        if (net_is_initialized()) {
            net_task();
        }

        watchdog_update();

        if (trigger_reset) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());

            if (current_time - last_reset_time < MIN_RESET_INTERVAL_MS) {
                trigger_reset = false;
                continue;
            }

            last_reset_time = current_time;
            trigger_reset = false;
            device_reset_in_progress = true;

            watchdog_update();

            uint8_t empty_kbd_report[8] = {0};
            uint8_t empty_mouse_report[7] = {0};
            uint16_t empty_consumer_report = 0;
            uint8_t empty_system_report = 0;

            for (int i = 0; i < 5; i++) {
                if (tud_hid_ready()) {
                    tud_hid_report(REPORT_ID_KEYBOARD, empty_kbd_report, sizeof(empty_kbd_report));
                    tud_hid_report(REPORT_ID_MOUSE, empty_mouse_report, sizeof(empty_mouse_report));
                    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_consumer_report, sizeof(empty_consumer_report));
                    tud_hid_report(REPORT_ID_SYSTEM_CONTROL, &empty_system_report, sizeof(empty_system_report));
                }
                sleep_ms(10);
                watchdog_update();
            }

            // Deinitialize network stack if active (must be done before USB deinit)
            if (net_is_initialized()) {
                net_deinit();
            }

            tud_disconnect();
            sleep_ms(RESET_DELAY_MS);
            watchdog_update();

            tud_deinit(0);
            sleep_ms(200);  // Increased delay for clean shutdown
            watchdog_update();

            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            mutex_enter_blocking(&state_mutex);
            if (connected_device_info.valid &&
                (current_time - connected_device_info.last_update_ms < DESCRIPTOR_TIMEOUT_MS)) {
                use_device_info = true;
            } else {
                use_device_info = false;
                reset_to_default_descriptors();
            }

            // Verify descriptor type is valid before reinitializing
            // Updated range to include network-only descriptor type
            if (current_descriptor_type > DESC_TYPE_NET_ONLY) {
                current_descriptor_type = DESC_TYPE_DEFAULT;
            }

            // Check if network mode will be enabled after reset
            bool will_enable_network = false;
            connected_device_status_t* device_status = get_connected_device_status();
            if (device_status && device_status->netbridge_mode_enabled) {
                will_enable_network = true;
            }
            mutex_exit(&state_mutex);

            watchdog_update();
            tud_init(0);
            sleep_ms(100);
            watchdog_update();
            tud_connect();

            // Initialize network stack if network mode is enabled
            if (will_enable_network) {
                // Wait for USB device to be fully enumerated and ready
                sleep_ms(200);
                watchdog_update();

                // Wait up to 2 seconds for TinyUSB to be ready
                uint32_t wait_start = to_ms_since_boot(get_absolute_time());
                while (!tud_ready() && (to_ms_since_boot(get_absolute_time()) - wait_start < 2000)) {
                    tud_task();
                    sleep_ms(10);
                    watchdog_update();
                }

                // Additional delay to ensure host has configured the device
                sleep_ms(500);
                watchdog_update();

                if (net_init()) {
                    // Network stack initialized successfully
                } else {
                    // Failed to initialize network stack - network may not work
                    // But don't fail the whole reset, just continue without network
                }
            }

            device_reset_in_progress = false;
        }
    }
    
    return 0;
}


void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    
    if (tud_hid_ready()) {
        uint8_t empty_report[8] = {0};
        tud_hid_report(REPORT_ID_KEYBOARD, empty_report, sizeof(empty_report));
        tud_hid_report(REPORT_ID_MOUSE, empty_report, sizeof(empty_report));
    }
}

void tud_resume_cb(void) {
    watchdog_update();
}

void tud_mount_cb(void) {
    mutex_enter_blocking(&queue_mutex);
    clear_report_queue();
    mutex_exit(&queue_mutex);
}

void tud_umount_cb(void) {
    uint8_t empty_kbd_report[8] = {0};
    uint8_t empty_mouse_report[7] = {0};
    uint16_t empty_consumer_report = 0;
    uint8_t empty_system_report = 0;
    
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_KEYBOARD, empty_kbd_report, sizeof(empty_kbd_report));
        tud_hid_report(REPORT_ID_MOUSE, empty_mouse_report, sizeof(empty_mouse_report));
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_consumer_report, sizeof(empty_consumer_report));
        tud_hid_report(REPORT_ID_SYSTEM_CONTROL, &empty_system_report, sizeof(empty_system_report));
    }
    
    mutex_enter_blocking(&queue_mutex);
    clear_report_queue();
    mutex_exit(&queue_mutex);
}

void tud_resume_remote_wakeup_cb(void) {}

void tud_set_configuration_cb(uint8_t configuration) {
    (void) configuration;
}

// CDC line coding callback - required for CDC device
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    (void) itf;
    (void) p_line_coding;
}

// CDC RX callback - forward serial CDC data from host PC to ESP8266 via UART
// CRITICAL: This callback is called from within tud_task() and MUST be non-blocking
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;

    connected_device_status_t* status = get_connected_device_status();

    // Only forward if serial CDC mode is enabled
    if (!status->serial_mode_enabled) {
        // Flush unwanted data
        uint32_t count = tud_cdc_available();
        if (count > 0) {
            uint8_t dummy[64];
            while (count > 0) {
                uint32_t to_read = (count > sizeof(dummy)) ? sizeof(dummy) : count;
                tud_cdc_read(dummy, to_read);
                count -= to_read;
            }
        }
        return;
    }

    // Read available data from CDC
    uint32_t count = tud_cdc_available();
    if (count == 0) return;

    // Check queue space - implement flow control
    uint16_t free_space = cdc_to_uart_get_free_space();

    // If queue is >75% full, skip reading to apply back-pressure
    if (free_space < (CDC_TO_UART_QUEUE_SIZE / 4)) {
        // Queue is critically full - don't read more data
        // Data will remain in CDC RX buffer and be processed later
        return;
    }

    // Process in chunks to avoid buffer overflow
    while (count > 0) {
        uint32_t to_read = (count > CDC_BUFFER_SIZE) ? CDC_BUFFER_SIZE : count;

        // Also limit by available queue space
        if (to_read > free_space) {
            to_read = free_space;
        }

        if (to_read == 0) {
            break;  // Queue full, stop reading
        }

        uint32_t actual_read = tud_cdc_read(cdc_buffer, to_read);

        if (actual_read > 0) {
            // Enqueue data for non-blocking forwarding to UART
            // Main loop will process this queue
            if (!cdc_to_uart_enqueue(cdc_buffer, actual_read)) {
                // Enqueue failed (queue full) - stop processing
                // Remaining data stays in CDC RX buffer
                break;
            }
            free_space -= actual_read;
        }

        count -= actual_read;
    }
}