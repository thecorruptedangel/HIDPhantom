#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "uart_comm.h"

static device_type_t detect_device_type(uint8_t dev_addr) {
    uint8_t keyboard_found = 0;
    uint8_t mouse_found = 0;

    for (uint8_t instance = 0; instance < CFG_TUH_HID; instance++) {
        if (tuh_hid_instance_count(dev_addr) > instance) {
            uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

            if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
                keyboard_found = 1;
            } else if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
                mouse_found = 1;
            }
        }
    }

    if (keyboard_found && mouse_found) {
        return DEVICE_TYPE_COMPOSITE;
    } else if (keyboard_found) {
        return DEVICE_TYPE_KEYBOARD;
    } else if (mouse_found) {
        return DEVICE_TYPE_MOUSE;
    } else {
        return DEVICE_TYPE_UNKNOWN;
    }
}

volatile bool device_connected = false;
volatile bool trigger_reset = false;
static uint8_t connected_dev_addr = 0;

mutex_t queue_mutex;
mutex_t state_mutex;


#define LANGUAGE_ID 0x0409

#define PIO_USB_CONFIG \
{ \
    PIO_USB_DP_PIN_DEFAULT, PIO_USB_TX_DEFAULT, PIO_SM_USB_TX_DEFAULT, \
    PIO_USB_DMA_TX_DEFAULT, PIO_USB_RX_DEFAULT, PIO_SM_USB_RX_DEFAULT, \
    PIO_SM_USB_EOP_DEFAULT, NULL, PIO_USB_DEBUG_PIN_NONE, \
    PIO_USB_DEBUG_PIN_NONE, false, PIO_USB_PINOUT_DPDM \
}

static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode);
static void process_kbd_report(uint8_t dev_addr, hid_keyboard_report_t const *report);
static void process_mouse_report(uint8_t dev_addr, uint8_t const * raw_report, uint16_t len);
static void reset_report_processing_state(void);

void core1_main() {
    sleep_ms(10);
    
    mutex_init(&queue_mutex);
    mutex_init(&state_mutex);
    
    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    
    tuh_init(1);

    // Core 1 should also update the watchdog to prevent false triggers
    extern void watchdog_update(void);

    while (true) {
        tuh_task();
        watchdog_update();  // Update watchdog on Core 1
        
        report_queue_entry_t entry;
        mutex_enter_blocking(&queue_mutex);
        bool has_report = dequeue_report(&entry);
        mutex_exit(&queue_mutex);
        
        if (has_report) {
            mutex_enter_blocking(&state_mutex);
            // In hybrid mode (DESC_TYPE_COMPOSITE) the virtual keyboard must work
            // even when no target device is connected.  Physical-device-only modes
            // still require a connected device so we don't forward stale reports.
            bool allow_virtual = (current_descriptor_type == DESC_TYPE_COMPOSITE);
            bool can_process = (device_connected || allow_virtual) && !device_reset_in_progress;
            mutex_exit(&state_mutex);

            if (can_process) {
                // Wait for HID to be ready before sending reports
                // This is especially important in storage mode (HID+MSC composite)
                if (!tud_hid_ready()) {
                    // Re-queue the report if HID is not ready
                    mutex_enter_blocking(&queue_mutex);
                    queue_report(&entry);
                    mutex_exit(&queue_mutex);
                    sleep_ms(1);  // Brief delay before retry
                } else {
                    switch(entry.interface_protocol) {
                        case HID_ITF_PROTOCOL_KEYBOARD:
                            if (entry.report_len >= sizeof(hid_keyboard_report_t)) {
                                hid_keyboard_report_t const* kbd_report = (hid_keyboard_report_t const*) entry.report;
                                if ((kbd_report->modifier & 0xFF) == kbd_report->modifier) {
                                    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, kbd_report->modifier, kbd_report->keycode);
                                    uart_send_hid_report(REPORT_ID_KEYBOARD, (const uint8_t*)kbd_report, sizeof(hid_keyboard_report_t));
                                }
                            }
                            break;

                        case HID_ITF_PROTOCOL_MOUSE:
                            if (entry.report_len >= 3) {
                                uint8_t buttons = entry.report[0];
                                int8_t x = (entry.report_len > 1) ? (int8_t)entry.report[1] : 0;
                                int8_t y = (entry.report_len > 2) ? (int8_t)entry.report[2] : 0;
                                int8_t wheel = (entry.report_len > 3) ? (int8_t)entry.report[3] : 0;

                                if (abs(x) <= 127 && abs(y) <= 127 && abs(wheel) <= 127) {
                                    tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, 0);
                                    uart_send_hid_report(REPORT_ID_MOUSE, entry.report, entry.report_len);
                                }
                            }
                            break;
                        
                        default:
                            if (entry.report_len > 0 && entry.report_len <= sizeof(entry.report)) {
                                if (entry.report[0] == REPORT_ID_CONSUMER_CONTROL && entry.report_len >= 3) {
                                    uint16_t usage_code = entry.report[1] | (entry.report[2] << 8);
                                    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &usage_code, 2);
                                    uart_send_hid_report(REPORT_ID_CONSUMER_CONTROL, (const uint8_t*)&usage_code, 2);
                                } else if (entry.report[0] == REPORT_ID_SYSTEM_CONTROL && entry.report_len >= 2) {
                                    uint8_t usage_code = entry.report[1];
                                    tud_hid_report(REPORT_ID_SYSTEM_CONTROL, &usage_code, 1);
                                    uart_send_hid_report(REPORT_ID_SYSTEM_CONTROL, &usage_code, 1);
                                }
                            }
                            break;
                    }
                }
            }
        }
    }
}

void tuh_mount_cb(uint8_t dev_addr) {
    mutex_enter_blocking(&state_mutex);
    
    if (connected_dev_addr != 0 && connected_dev_addr != dev_addr) {
        mutex_exit(&state_mutex);
        return;
    }
    
    reset_report_processing_state();
    
    uint16_t vid, pid;
    if (!tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        mutex_exit(&state_mutex);
        return;
    }
    
    char mfg_str[64] = {0};
    char product_str[64] = {0};
    char serial_str[64] = {0};
    
    tuh_descriptor_get_manufacturer_string_sync(dev_addr, LANGUAGE_ID, mfg_str, sizeof(mfg_str) - 1);
    tuh_descriptor_get_product_string_sync(dev_addr, LANGUAGE_ID, product_str, sizeof(product_str) - 1);
    tuh_descriptor_get_serial_string_sync(dev_addr, LANGUAGE_ID, serial_str, sizeof(serial_str) - 1);
    
    tusb_desc_device_t desc_device;
    uint16_t bcdDevice = 0x0100;
    if (tuh_descriptor_get_device_sync(dev_addr, &desc_device, sizeof(desc_device)) == XFER_RESULT_SUCCESS) {
        bcdDevice = desc_device.bcdDevice;
    }
    
    update_device_info(vid, pid, mfg_str, product_str, serial_str, bcdDevice);
    connected_dev_addr = dev_addr;

    // Update device status for UART commands
    connected_device_status_t* status = get_connected_device_status();
    device_type_t detected_type = detect_device_type(dev_addr);

    status->vid = vid;
    status->pid = pid;
    strncpy(status->manufacturer, mfg_str, sizeof(status->manufacturer) - 1);
    status->manufacturer[sizeof(status->manufacturer) - 1] = '\0';
    strncpy(status->product, product_str, sizeof(status->product) - 1);
    status->product[sizeof(status->product) - 1] = '\0';
    strncpy(status->serial, serial_str, sizeof(status->serial) - 1);
    status->serial[sizeof(status->serial) - 1] = '\0';
    status->country_code = 0; // Will be updated when keyboard descriptor is parsed
    status->connected = true;

    // Always store the actual detected device type
    status->actual_device_type = detected_type;
    status->type = detected_type;

    // Set appropriate descriptor type based on hybrid mode
    descriptor_type_t desc_type = DESC_TYPE_DEFAULT;
    if (status->hybrid_mode_enabled) {
        // Hybrid mode always uses composite descriptor regardless of actual device type
        desc_type = DESC_TYPE_COMPOSITE;
    } else {
        // True spoofing mode - match descriptor to actual device type
        switch (detected_type) {
            case DEVICE_TYPE_KEYBOARD:
                desc_type = DESC_TYPE_KEYBOARD_ONLY;
                break;
            case DEVICE_TYPE_MOUSE:
                desc_type = DESC_TYPE_MOUSE_ONLY;
                break;
            case DEVICE_TYPE_COMPOSITE:
                desc_type = DESC_TYPE_COMPOSITE;
                break;
            default:
                desc_type = DESC_TYPE_DEFAULT;
                break;
        }
    }
    set_descriptor_type(desc_type);

    trigger_reset = true;

    mutex_exit(&state_mutex);

    // Send device status update to ESP8266
    uart_send_device_status_update();
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    // Parse keyboard descriptor for country code
    if (desc_report && desc_len > 0) {
        uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
        if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
            // Try to extract country code from HID descriptor
            // Country code is typically at offset 7 in the HID descriptor
            connected_device_status_t* status = get_connected_device_status();
            if (desc_len > 7) {
                // This is a simplified approach - real parsing would be more complex
                status->country_code = 0; // Default to US layout
                // Note: Proper country code extraction requires parsing the full HID descriptor
            }
        }
    }
    
    if (instance >= CFG_TUH_HID) {
        return;
    }
    
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    
    mutex_enter_blocking(&state_mutex);
    device_connected = true;
    mutex_exit(&state_mutex);
    
    
    if (itf_protocol != HID_ITF_PROTOCOL_NONE) {
        tuh_hid_receive_report(dev_addr, instance);
    }
}

void tuh_umount_cb(uint8_t dev_addr) {
    mutex_enter_blocking(&state_mutex);
    
    if (dev_addr == connected_dev_addr) {
        device_connected = false;
        connected_dev_addr = 0;
        
        
        // Clear device-specific fields but preserve user-configured mode flags so
        // a hot-unplug does not silently disable the network bridge or hybrid mode.
        connected_device_status_t* status = get_connected_device_status();
        bool preserve_hybrid    = status->hybrid_mode_enabled;
        bool preserve_serial    = status->serial_mode_enabled;
        bool preserve_storage   = status->storage_mode_enabled;
        bool preserve_netbridge = status->netbridge_mode_enabled;
        memset(status, 0, sizeof(connected_device_status_t));
        status->connected              = false;
        status->hybrid_mode_enabled    = preserve_hybrid;
        status->serial_mode_enabled    = preserve_serial;
        status->storage_mode_enabled   = preserve_storage;
        status->netbridge_mode_enabled = preserve_netbridge;

        // Reset to default descriptor
        set_descriptor_type(DESC_TYPE_DEFAULT);

        reset_all_state();
        trigger_reset = true;

        mutex_exit(&state_mutex);

        // Send device disconnected notification to ESP8266
        uart_send_device_status_update();
    } else {
        mutex_exit(&state_mutex);
    }
}

static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode) {
    for(uint8_t i = 0; i < 6; i++) {
        if (report->keycode[i] == keycode) return true;
    }
    return false;
}

static void process_kbd_report(uint8_t dev_addr, hid_keyboard_report_t const *report) {
    (void) dev_addr;
    
    mutex_enter_blocking(&state_mutex);
    bool can_process = device_connected && !device_reset_in_progress;
    mutex_exit(&state_mutex);
    
    if (can_process) {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report->modifier, report->keycode);
        uart_send_hid_report(REPORT_ID_KEYBOARD, (const uint8_t*)report, sizeof(hid_keyboard_report_t));
    }
}

static void process_mouse_report(uint8_t dev_addr, uint8_t const * raw_report, uint16_t len) {
    (void) dev_addr;
    
    mutex_enter_blocking(&state_mutex);
    bool can_process = device_connected && !device_reset_in_progress;
    mutex_exit(&state_mutex);
    
    if (can_process) {
        uint8_t buttons = 0;
        int8_t x = 0, y = 0, wheel = 0;
        
        if (len >= 1) buttons = raw_report[0];
        if (len >= 2) x = (int8_t)raw_report[1];
        if (len >= 3) y = (int8_t)raw_report[2];
        if (len >= 4) wheel = (int8_t)raw_report[3];
        
        tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, 0);

        uint8_t mouse_data[4] = {buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel};
        uart_send_hid_report(REPORT_ID_MOUSE, mouse_data, 4);
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (report == NULL || len == 0 || instance >= CFG_TUH_HID) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }
    
    mutex_enter_blocking(&state_mutex);
    bool can_process = device_connected && !device_reset_in_progress;
    mutex_exit(&state_mutex);
    
    if (!can_process) {
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }
    
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    
    switch(itf_protocol) {
        case HID_ITF_PROTOCOL_KEYBOARD:
            process_kbd_report(dev_addr, (hid_keyboard_report_t const*) report);
            break;
            
        case HID_ITF_PROTOCOL_MOUSE:
            process_mouse_report(dev_addr, report, len);
            break;
            
        default:
            if (len > 0 && len <= 64) {
                if (report[0] < REPORT_ID_COUNT) {
                    report_queue_entry_t entry = {0};
                    entry.dev_addr = dev_addr;
                    entry.instance = instance;
                    entry.interface_protocol = itf_protocol;
                    memcpy(entry.report, report, len);
                    entry.report_len = len;
                    
                    mutex_enter_blocking(&queue_mutex);
                    queue_report(&entry);
                    mutex_exit(&queue_mutex);
                    
                }
            }
            break;
    }
    
    tuh_hid_receive_report(dev_addr, instance);
}

static void reset_report_processing_state(void) {
    mutex_enter_blocking(&queue_mutex);
    clear_report_queue();
    mutex_exit(&queue_mutex);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void) dev_addr;
    (void) instance;
}
