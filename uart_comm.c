#include "uart_comm.h"
#include "usb_descriptors.h"
#include "pico/mutex.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static uart_buffer_t rx_buffer = {0};
static volatile bool uart_initialized = false;
static connected_device_status_t device_status = {0};

// ---- Unified UART TX queue (Core-0 drains, any core enqueues under mutex) ----
static uart_tx_queue_t uart_tx_queue = {0};
static mutex_t         uart_tx_mutex;

// CDC to UART queue for non-blocking forwarding
static cdc_to_uart_queue_t cdc_to_uart_queue = {0};
static volatile bool cdc_to_uart_overflow_occurred = false;

// UART to CDC queue for non-blocking forwarding
static uart_to_cdc_queue_t uart_to_cdc_queue = {0};
static volatile bool uart_to_cdc_overflow_occurred = false;

// Network packet callback for forwarding to lwIP stack
static net_packet_callback_t net_packet_callback = NULL;

extern mutex_t queue_mutex;
extern mutex_t state_mutex;
extern volatile bool device_connected;
extern volatile bool device_reset_in_progress;

static void uart_rx_interrupt_handler(void) {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);

        if (rx_buffer.count < UART_BUFFER_SIZE) {
            rx_buffer.buffer[rx_buffer.tail] = ch;
            rx_buffer.tail = (rx_buffer.tail + 1) % UART_BUFFER_SIZE;
            rx_buffer.count++;
        }
    }
}

void uart_comm_init(void) {
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);  // Enable FIFO for 32-byte hardware buffering

    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, uart_rx_interrupt_handler);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    memset(&rx_buffer, 0, sizeof(rx_buffer));

    // Unified TX queue — must be ready before Core 1 starts
    memset(&uart_tx_queue, 0, sizeof(uart_tx_queue));
    mutex_init(&uart_tx_mutex);

    // Initialize queues
    cdc_to_uart_queue_init();
    uart_to_cdc_queue_init();

    uart_initialized = true;
}

// ========== CDC TO UART QUEUE MANAGEMENT ==========

void cdc_to_uart_queue_init(void) {
    memset(&cdc_to_uart_queue, 0, sizeof(cdc_to_uart_queue));
    cdc_to_uart_overflow_occurred = false;
}

bool cdc_to_uart_enqueue(const uint8_t* data, uint16_t len) {
    if (!uart_initialized || data == NULL || len == 0) {
        return false;
    }

    // Check if queue has enough space
    uint16_t free_space = CDC_TO_UART_QUEUE_SIZE - cdc_to_uart_queue.count;
    if (len > free_space) {
        cdc_to_uart_overflow_occurred = true;
        return false;  // Queue full - cannot enqueue
    }

    // Enqueue data
    for (uint16_t i = 0; i < len; i++) {
        cdc_to_uart_queue.buffer[cdc_to_uart_queue.tail] = data[i];
        cdc_to_uart_queue.tail = (cdc_to_uart_queue.tail + 1) % CDC_TO_UART_QUEUE_SIZE;
        cdc_to_uart_queue.count++;
    }

    return true;
}

uint16_t cdc_to_uart_get_free_space(void) {
    return CDC_TO_UART_QUEUE_SIZE - cdc_to_uart_queue.count;
}

void process_cdc_to_uart_queue(void) {
    if (!uart_initialized || cdc_to_uart_queue.count == 0) {
        return;
    }

    extern void watchdog_update(void);

    // Process bytes while UART is writable
    uint16_t bytes_written = 0;
    while (cdc_to_uart_queue.count > 0 && uart_is_writable(UART_ID)) {
        uint8_t byte = cdc_to_uart_queue.buffer[cdc_to_uart_queue.head];
        uart_putc_raw(UART_ID, byte);  // Use raw putc since FIFO is enabled

        cdc_to_uart_queue.head = (cdc_to_uart_queue.head + 1) % CDC_TO_UART_QUEUE_SIZE;
        cdc_to_uart_queue.count--;
        bytes_written++;

        // Update watchdog every 32 bytes to prevent timeout
        if (bytes_written % 32 == 0) {
            watchdog_update();
        }
    }

    // Report overflow if occurred (one-time warning)
    if (cdc_to_uart_overflow_occurred) {
        cdc_to_uart_overflow_occurred = false;
        uart_send_response("WARN: CDC_TO_UART_QUEUE_OVERFLOW");
    }
}

// ========== UART TO CDC QUEUE MANAGEMENT ==========

void uart_to_cdc_queue_init(void) {
    memset(&uart_to_cdc_queue, 0, sizeof(uart_to_cdc_queue));
    uart_to_cdc_overflow_occurred = false;
}

bool uart_to_cdc_enqueue(const uint8_t* data, uint16_t len) {
    if (data == NULL || len == 0) {
        return false;
    }

    // Check if queue has enough space
    uint16_t free_space = UART_TO_CDC_QUEUE_SIZE - uart_to_cdc_queue.count;
    if (len > free_space) {
        uart_to_cdc_overflow_occurred = true;
        return false;  // Queue full - cannot enqueue
    }

    // Enqueue data
    for (uint16_t i = 0; i < len; i++) {
        uart_to_cdc_queue.buffer[uart_to_cdc_queue.tail] = data[i];
        uart_to_cdc_queue.tail = (uart_to_cdc_queue.tail + 1) % UART_TO_CDC_QUEUE_SIZE;
        uart_to_cdc_queue.count++;
    }

    return true;
}

uint16_t uart_to_cdc_get_free_space(void) {
    return UART_TO_CDC_QUEUE_SIZE - uart_to_cdc_queue.count;
}

void process_uart_to_cdc_queue(void) {
    if (uart_to_cdc_queue.count == 0) {
        return;
    }

    // Only process if USB device is mounted (enumerated)
    // Don't check DTR (tud_cdc_connected) - send data as soon as device enumerates
    if (!tud_mounted()) {
        return;  // Keep data in queue until device is enumerated
    }

    extern void watchdog_update(void);

    // Process bytes while CDC TX buffer has space
    uint16_t bytes_written = 0;
    while (uart_to_cdc_queue.count > 0) {
        uint32_t available = tud_cdc_write_available();
        if (available == 0) {
            break;  // CDC TX buffer full, will retry on next call
        }

        // Write bytes to CDC
        uint16_t batch_size = (uart_to_cdc_queue.count < available) ? uart_to_cdc_queue.count : available;
        for (uint16_t i = 0; i < batch_size; i++) {
            uint8_t byte = uart_to_cdc_queue.buffer[uart_to_cdc_queue.head];
            tud_cdc_write(&byte, 1);

            uart_to_cdc_queue.head = (uart_to_cdc_queue.head + 1) % UART_TO_CDC_QUEUE_SIZE;
            uart_to_cdc_queue.count--;
            bytes_written++;

            // Update watchdog every 32 bytes to prevent timeout
            if (bytes_written % 32 == 0) {
                watchdog_update();
            }
        }

        // Flush after each batch
        tud_cdc_write_flush();
    }

    // Report overflow if occurred (one-time warning)
    if (uart_to_cdc_overflow_occurred) {
        uart_to_cdc_overflow_occurred = false;
        uart_send_response("WARN: UART_TO_CDC_QUEUE_OVERFLOW");
    }
}

// ========== ORIGINAL UART FUNCTIONS ==========

// ---- Internal: enqueue bytes into TX queue, caller must hold uart_tx_mutex ----
static void tx_enqueue_locked(const uint8_t* data, uint16_t len) {
    uint16_t free = UART_TX_QUEUE_SIZE - uart_tx_queue.count;
    if (len > free) return;  // drop rather than partial-write a frame
    for (uint16_t i = 0; i < len; i++) {
        uart_tx_queue.buffer[uart_tx_queue.tail] = data[i];
        uart_tx_queue.tail = (uart_tx_queue.tail + 1) % UART_TX_QUEUE_SIZE;
        uart_tx_queue.count++;
    }
}

// Intercept copy of physical keyboard/mouse events sent to ESP8266 for logging.
// The ACTUAL keyboard→host path (tud_hid_keyboard_report) runs over USB and is
// completely unaffected by this function or any UART contention.
void uart_send_hid_report(uint8_t report_id, const uint8_t* data, uint16_t len) {
    if (!uart_initialized || !data || len == 0) return;

    // Build full text frame in a local buffer — stack-safe (max ~155 bytes)
    uint8_t frame[160];
    uint16_t pos = 0;
    pos += (uint16_t)snprintf((char*)frame + pos, sizeof(frame) - pos,
                              "HID:%02X:%04X:", report_id, len);
    for (uint16_t i = 0; i < len && pos < sizeof(frame) - 2; i++) {
        pos += (uint16_t)snprintf((char*)frame + pos, sizeof(frame) - pos,
                                  "%02X", data[i]);
    }
    if (pos < sizeof(frame) - 1) { frame[pos++] = '\r'; }
    if (pos < sizeof(frame))     { frame[pos++] = '\n'; }

    mutex_enter_blocking(&uart_tx_mutex);
    tx_enqueue_locked(frame, pos);
    mutex_exit(&uart_tx_mutex);
}

// Network packet from USB host → SLIP-encode and queue for ESP8266.
// Runs on Core 0; uart_send_hid_report runs on Core 1 — mutex prevents interleave.
void uart_send_net_packet(const uint8_t* packet, uint16_t len) {
    if (!uart_initialized || !packet || len == 0 || len > SLIP_MAX_FRAME) return;

    // Worst-case encoded size: each byte becomes 2 after escaping, plus 2 END markers + 1 type byte
    uint16_t worst = len * 2 + 3;

    mutex_enter_blocking(&uart_tx_mutex);

    if (UART_TX_QUEUE_SIZE - uart_tx_queue.count < worst) {
        mutex_exit(&uart_tx_mutex);
        return;  // queue full — drop packet, correct backpressure
    }

    // Write SLIP frame directly into queue under mutex (avoids stack allocation of ~3KB)
#define TXQ_PUT(b) do { \
    uart_tx_queue.buffer[uart_tx_queue.tail] = (b); \
    uart_tx_queue.tail = (uart_tx_queue.tail + 1) % UART_TX_QUEUE_SIZE; \
    uart_tx_queue.count++; \
} while(0)

    TXQ_PUT(SLIP_END);
    TXQ_PUT(SLIP_TYPE_NET);  // Frame-type discriminator — lets ESP8266 distinguish NET from image frames
    for (uint16_t i = 0; i < len; i++) {
        if (packet[i] == SLIP_END) {
            TXQ_PUT(SLIP_ESC); TXQ_PUT(SLIP_ESC_END);
        } else if (packet[i] == SLIP_ESC) {
            TXQ_PUT(SLIP_ESC); TXQ_PUT(SLIP_ESC_ESC);
        } else {
            TXQ_PUT(packet[i]);
        }
    }
    TXQ_PUT(SLIP_END);
#undef TXQ_PUT

    mutex_exit(&uart_tx_mutex);
}

void uart_send_response(const char* response) {
    if (!uart_initialized || !response) return;
    uint16_t len = (uint16_t)strlen(response);
    mutex_enter_blocking(&uart_tx_mutex);
    tx_enqueue_locked((const uint8_t*)response, len);
    uint8_t crlf[2] = {'\r', '\n'};
    tx_enqueue_locked(crlf, 2);
    mutex_exit(&uart_tx_mutex);
}

// Called from Core 0 main loop — drains one 32-byte batch from the TX queue.
// One batch per call keeps Core 0 blocked for at most ~0.35ms (one FIFO depth at
// 921600 baud). The main loop runs far faster than the UART byte rate so the full
// queue drains across successive iterations without losing throughput.
void uart_drain_tx_queue(void) {
    if (!uart_initialized) return;
    uint8_t batch[32];
    uint16_t n;

    mutex_enter_blocking(&uart_tx_mutex);
    n = 0;
    while (n < sizeof(batch) && uart_tx_queue.count > 0) {
        batch[n++] = uart_tx_queue.buffer[uart_tx_queue.head];
        uart_tx_queue.head = (uart_tx_queue.head + 1) % UART_TX_QUEUE_SIZE;
        uart_tx_queue.count--;
    }
    mutex_exit(&uart_tx_mutex);

    for (uint16_t i = 0; i < n; i++) {
        uart_putc_raw(UART_ID, batch[i]);
    }
}

void uart_set_net_packet_callback(net_packet_callback_t callback) {
    net_packet_callback = callback;
}

connected_device_status_t* get_connected_device_status(void) {
    return &device_status;
}

void uart_send_device_status_update(void) {
    // Send device status as +INFO: response for ESP8266 to update UI in real-time
    char response[400];
    if (device_status.connected) {
        const char* type_str = "UNKNOWN";
        // Report HYBRID as type when hybrid mode is enabled for UI display
        if (device_status.hybrid_mode_enabled) {
            type_str = "HYBRID";
        } else {
            switch (device_status.actual_device_type) {
                case DEVICE_TYPE_KEYBOARD: type_str = "KEYBOARD"; break;
                case DEVICE_TYPE_MOUSE: type_str = "MOUSE"; break;
                case DEVICE_TYPE_COMPOSITE: type_str = "COMPOSITE"; break;
                default: type_str = "UNKNOWN"; break;
            }
        }
        snprintf(response, sizeof(response),
            "+INFO: TYPE=%s,VID=%04X,PID=%04X,MFG=%.63s,PROD=%.63s,SER=%.63s,HYBRID=%s,SERIAL=%s,STORAGE=%s,NETBRIDGE=%s",
            type_str, device_status.vid, device_status.pid,
            device_status.manufacturer, device_status.product,
            device_status.serial,
            device_status.hybrid_mode_enabled ? "ON" : "OFF",
            device_status.serial_mode_enabled ? "ON" : "OFF",
            device_status.storage_mode_enabled ? "ON" : "OFF",
            device_status.netbridge_mode_enabled ? "ON" : "OFF");
    } else {
        snprintf(response, sizeof(response), "+INFO: TYPE=NONE,CONNECTED=FALSE,HYBRID=%s,SERIAL=%s,STORAGE=%s,NETBRIDGE=%s",
            device_status.hybrid_mode_enabled ? "ON" : "OFF",
            device_status.serial_mode_enabled ? "ON" : "OFF",
            device_status.storage_mode_enabled ? "ON" : "OFF",
            device_status.netbridge_mode_enabled ? "ON" : "OFF");
    }
    uart_send_response(response);
}

// Helper function to determine the correct descriptor type based on current state
static descriptor_type_t get_appropriate_descriptor_type(void) {
    extern void set_descriptor_type(descriptor_type_t type);

    // Priority: NET + HID composite > CDC + MSC > CDC > MSC > base HID type
    bool cdc_enabled = device_status.serial_mode_enabled;
    bool msc_enabled = device_status.storage_mode_enabled;
    bool net_enabled = device_status.netbridge_mode_enabled;

    // Network mode uses network-only descriptor for maximum Windows compatibility
    // HID reports still forwarded over UART to ESP8266
    if (net_enabled) {
        return DESC_TYPE_NET_ONLY;
    }

    // If both CDC and MSC are enabled (no network)
    if (cdc_enabled && msc_enabled) {
        return DESC_TYPE_HID_CDC_MSC_COMPOSITE;
    }

    // If only CDC is enabled
    if (cdc_enabled) {
        return DESC_TYPE_HID_CDC_COMPOSITE;
    }

    // If only MSC is enabled
    if (msc_enabled) {
        return DESC_TYPE_HID_MSC_COMPOSITE;
    }

    // No CDC, MSC, or network - determine base HID type
    if (device_status.hybrid_mode_enabled) {
        // Hybrid mode always uses composite, regardless of device connection
        return DESC_TYPE_COMPOSITE;
    } else if (device_status.connected) {
        // Device connected - match descriptor to actual device type
        // Safety check: if actual_device_type is not set, fall back to type field
        device_type_t actual_type = device_status.actual_device_type;
        if (actual_type == DEVICE_TYPE_NONE || actual_type == DEVICE_TYPE_UNKNOWN) {
            actual_type = device_status.type;
        }

        switch (actual_type) {
            case DEVICE_TYPE_KEYBOARD:
                return DESC_TYPE_KEYBOARD_ONLY;
            case DEVICE_TYPE_MOUSE:
                return DESC_TYPE_MOUSE_ONLY;
            case DEVICE_TYPE_COMPOSITE:
                return DESC_TYPE_COMPOSITE;
            default:
                return DESC_TYPE_DEFAULT;
        }
    }

    // No device connected and no special modes - use default
    return DESC_TYPE_DEFAULT;
}

static bool parse_hid_command(const char* cmd, uint8_t* report_id, uint8_t* data, uint16_t* len) {
    if (strncmp(cmd, "HID:", 4) != 0) return false;

    char* ptr = (char*)cmd + 4;
    char* end_ptr;

    *report_id = (uint8_t)strtol(ptr, &end_ptr, 16);
    if (*end_ptr != ':') return false;

    ptr = end_ptr + 1;
    *len = (uint16_t)strtol(ptr, &end_ptr, 16);
    if (*end_ptr != ':') return false;

    ptr = end_ptr + 1;

    if (*len > 64) return false;

    for (uint16_t i = 0; i < *len; i++) {
        if (strlen(ptr) < 2) return false;

        char hex_byte[3] = {ptr[0], ptr[1], '\0'};
        data[i] = (uint8_t)strtol(hex_byte, NULL, 16);
        ptr += 2;
    }

    return true;
}

// SLIP receive state — persistent across calls
static uint8_t  slip_buf[SLIP_MAX_FRAME];
static uint16_t slip_len       = 0;
static bool     slip_in_frame  = false;
static bool     slip_escape    = false;

static void slip_dispatch(void) {
    if (slip_len > 0 && net_packet_callback && device_status.netbridge_mode_enabled) {
        net_packet_callback(slip_buf, slip_len);
    }
    slip_len = 0;
    slip_escape = false;
}

static void process_at_command(const char* cmd) {
    // Don't process mode-changing commands during USB reset to prevent race conditions
    // Query commands and AT ping are still allowed
    if (device_reset_in_progress) {
        if (strcmp(cmd, "AT") == 0 ||
            strcmp(cmd, "AT+INFO") == 0 ||
            strcmp(cmd, "AT+TYPE") == 0 ||
            strcmp(cmd, "AT+HYBRID?") == 0 ||
            strcmp(cmd, "AT+SERIAL?") == 0 ||
            strcmp(cmd, "AT+STORAGE?") == 0 ||
            strcmp(cmd, "AT+NETBRIDGE?") == 0) {
            // Allow read-only queries during reset
        } else {
            // Silently ignore mode-changing commands during reset
            return;
        }
    }

    if (strcmp(cmd, "AT") == 0) {
        uart_send_response("OK");
    } else if (strcmp(cmd, "AT+INFO") == 0) {
        char response[400];  // Increased to accommodate storage mode field
        if (device_status.connected) {
            const char* type_str = "UNKNOWN";
            // Report HYBRID as type when hybrid mode is enabled for UI display
            if (device_status.hybrid_mode_enabled) {
                type_str = "HYBRID";
            } else {
                switch (device_status.actual_device_type) {
                    case DEVICE_TYPE_KEYBOARD: type_str = "KEYBOARD"; break;
                    case DEVICE_TYPE_MOUSE: type_str = "MOUSE"; break;
                    case DEVICE_TYPE_COMPOSITE: type_str = "COMPOSITE"; break;
                    default: type_str = "UNKNOWN"; break;
                }
            }
            // Use snprintf with proper size to prevent overflow
            snprintf(response, sizeof(response),
                "+INFO: TYPE=%s,VID=%04X,PID=%04X,MFG=%.63s,PROD=%.63s,SER=%.63s,HYBRID=%s,SERIAL=%s,STORAGE=%s,NETBRIDGE=%s",
                type_str, device_status.vid, device_status.pid,
                device_status.manufacturer, device_status.product,
                device_status.serial,
                device_status.hybrid_mode_enabled ? "ON" : "OFF",
                device_status.serial_mode_enabled ? "ON" : "OFF",
                device_status.storage_mode_enabled ? "ON" : "OFF",
                device_status.netbridge_mode_enabled ? "ON" : "OFF");
        } else {
            snprintf(response, sizeof(response), "+INFO: TYPE=NONE,CONNECTED=FALSE,HYBRID=%s,SERIAL=%s,STORAGE=%s,NETBRIDGE=%s",
                device_status.hybrid_mode_enabled ? "ON" : "OFF",
                device_status.serial_mode_enabled ? "ON" : "OFF",
                device_status.storage_mode_enabled ? "ON" : "OFF",
                device_status.netbridge_mode_enabled ? "ON" : "OFF");
        }
        uart_send_response(response);
    } else if (strcmp(cmd, "AT+TYPE") == 0) {
        if (device_status.connected) {
            const char* type_str = "UNKNOWN";
            // Report HYBRID as type when hybrid mode is enabled for UI display
            if (device_status.hybrid_mode_enabled) {
                type_str = "HYBRID";
            } else {
                switch (device_status.actual_device_type) {
                    case DEVICE_TYPE_KEYBOARD: type_str = "KEYBOARD"; break;
                    case DEVICE_TYPE_MOUSE: type_str = "MOUSE"; break;
                    case DEVICE_TYPE_COMPOSITE: type_str = "COMPOSITE"; break;
                    default: type_str = "UNKNOWN"; break;
                }
            }
            char response[32];
            snprintf(response, sizeof(response), "+TYPE: %s", type_str);
            uart_send_response(response);
        } else {
            uart_send_response("+TYPE: NONE");
        }
    } else if (strcmp(cmd, "AT+HYBRID=ON") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.hybrid_mode_enabled;
        device_status.hybrid_mode_enabled = true;

        // Only trigger reset and respond when hybrid mode actually changes
        if (!was_enabled) {
            // Ensure actual_device_type is set correctly if device is connected
            if (device_status.connected && device_status.actual_device_type == DEVICE_TYPE_NONE) {
                device_status.actual_device_type = device_status.type;
            }

            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            // Use appropriate composite descriptor (may include CDC/MSC if enabled)
            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+HYBRID: ON");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+HYBRID=OFF") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.hybrid_mode_enabled;
        device_status.hybrid_mode_enabled = false;

        // Only trigger reset and respond when hybrid mode actually changes
        if (was_enabled) {
            // Ensure actual_device_type is properly set if device is connected
            if (device_status.connected) {
                if (device_status.actual_device_type == DEVICE_TYPE_NONE ||
                    device_status.actual_device_type == DEVICE_TYPE_UNKNOWN) {
                    device_status.actual_device_type = device_status.type;
                }
            }

            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+HYBRID: OFF");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+HYBRID?") == 0) {
        char response[32];
        snprintf(response, sizeof(response), "+HYBRID: %s", device_status.hybrid_mode_enabled ? "ON" : "OFF");
        uart_send_response(response);
    } else if (strcmp(cmd, "AT+SERIAL=ON") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.serial_mode_enabled;
        device_status.serial_mode_enabled = true;

        // Only trigger reset and respond when serial mode actually changes
        if (!was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+SERIAL: ON");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+SERIAL=OFF") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.serial_mode_enabled;
        device_status.serial_mode_enabled = false;

        // Only trigger reset and respond when serial mode actually changes
        if (was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+SERIAL: OFF");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+SERIAL?") == 0) {
        char response[32];
        snprintf(response, sizeof(response), "+SERIAL: %s", device_status.serial_mode_enabled ? "ON" : "OFF");
        uart_send_response(response);
    } else if (strcmp(cmd, "AT+STORAGE=ON") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.storage_mode_enabled;
        device_status.storage_mode_enabled = true;

        // Only trigger reset and respond when storage mode actually changes
        if (!was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+STORAGE: ON");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+STORAGE=OFF") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.storage_mode_enabled;
        device_status.storage_mode_enabled = false;

        // Only trigger reset and respond when storage mode actually changes
        if (was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+STORAGE: OFF");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+STORAGE?") == 0) {
        char response[32];
        snprintf(response, sizeof(response), "+STORAGE: %s", device_status.storage_mode_enabled ? "ON" : "OFF");
        uart_send_response(response);
    } else if (strcmp(cmd, "AT+NETBRIDGE=ON") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.netbridge_mode_enabled;
        device_status.netbridge_mode_enabled = true;

        // Only trigger reset and respond when network bridge mode actually changes
        if (!was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+NETBRIDGE: ON");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+NETBRIDGE=OFF") == 0) {
        extern volatile bool trigger_reset;
        extern void set_descriptor_type(descriptor_type_t type);

        mutex_enter_blocking(&state_mutex);
        bool was_enabled = device_status.netbridge_mode_enabled;
        device_status.netbridge_mode_enabled = false;

        // Only trigger reset and respond when network bridge mode actually changes
        if (was_enabled) {
            // Clear report queue before descriptor change
            mutex_enter_blocking(&queue_mutex);
            clear_report_queue();
            mutex_exit(&queue_mutex);

            descriptor_type_t desc_type = get_appropriate_descriptor_type();
            set_descriptor_type(desc_type);
            trigger_reset = true;
            mutex_exit(&state_mutex);

            // Send response only when state changed
            uart_send_response("+NETBRIDGE: OFF");
        } else {
            // Already in requested state - no response to prevent spam
            mutex_exit(&state_mutex);
        }
    } else if (strcmp(cmd, "AT+NETBRIDGE?") == 0) {
        char response[32];
        snprintf(response, sizeof(response), "+NETBRIDGE: %s", device_status.netbridge_mode_enabled ? "ON" : "OFF");
        uart_send_response(response);
    } else {
        uart_send_response("ERROR: UNKNOWN_CMD");
    }
}

static void process_uart_hid_command(uint8_t report_id, const uint8_t* data, uint16_t len) {
    mutex_enter_blocking(&state_mutex);
    bool can_process = device_connected && !device_reset_in_progress;
    device_type_t actual_type = device_status.actual_device_type;
    bool hybrid_enabled = device_status.hybrid_mode_enabled;
    mutex_exit(&state_mutex);

    if (!can_process) return;

    // Validate HID command against actual device type
    // Only allow mismatched commands if hybrid mode is enabled or device is composite
    if (actual_type != DEVICE_TYPE_COMPOSITE && !hybrid_enabled) {
        bool is_keyboard_command = (report_id == REPORT_ID_KEYBOARD ||
                                   report_id == REPORT_ID_CONSUMER_CONTROL ||
                                   report_id == REPORT_ID_SYSTEM_CONTROL);
        bool is_mouse_command = (report_id == REPORT_ID_MOUSE);

        // Block mismatched commands when hybrid mode is disabled
        if (actual_type == DEVICE_TYPE_KEYBOARD && is_mouse_command) {
            uart_send_response("ERROR: MOUSE_CMD_REQUIRES_HYBRID");
            return;
        } else if (actual_type == DEVICE_TYPE_MOUSE && is_keyboard_command) {
            uart_send_response("ERROR: KEYBOARD_CMD_REQUIRES_HYBRID");
            return;
        }
    }

    switch (report_id) {
        case REPORT_ID_KEYBOARD:
            if (len >= sizeof(hid_keyboard_report_t)) {
                const hid_keyboard_report_t* kbd_report = (const hid_keyboard_report_t*)data;
                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, kbd_report->modifier, kbd_report->keycode);
            }
            break;

        case REPORT_ID_MOUSE:
            if (len >= 3) {
                uint8_t buttons = data[0];
                int8_t x = (len > 1) ? (int8_t)data[1] : 0;
                int8_t y = (len > 2) ? (int8_t)data[2] : 0;
                int8_t wheel = (len > 3) ? (int8_t)data[3] : 0;
                tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, 0);
            }
            break;

        case REPORT_ID_CONSUMER_CONTROL:
            if (len >= 2) {
                uint16_t usage_code = data[0] | (data[1] << 8);
                tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &usage_code, 2);
            }
            break;

        case REPORT_ID_SYSTEM_CONTROL:
            if (len >= 1) {
                uint8_t usage_code = data[0];
                tud_hid_report(REPORT_ID_SYSTEM_CONTROL, &usage_code, 1);
            }
            break;

        default:
            if (len > 0) {
                tud_hid_report(report_id, data, len);
            }
            break;
    }
}

bool uart_receive_data(uint8_t* buffer, uint16_t* len) {
    if (!uart_initialized) {
        *len = 0;
        return false;
    }

    // Cortex-M0+ has no atomic RMW: count-- is three instructions (LDRH/SUB/STRH)
    // and the UART ISR can increment count between them, losing its update.
    // Mask the UART IRQ for the two short critical sections (read + write of count).
    const uint irq_num = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    irq_set_enabled(irq_num, false);
    uint16_t available = rx_buffer.count;
    irq_set_enabled(irq_num, true);

    if (available == 0) {
        *len = 0;
        return false;
    }
    if (available > *len) available = *len;

    for (uint16_t i = 0; i < available; i++) {
        buffer[i] = rx_buffer.buffer[rx_buffer.head];
        rx_buffer.head = (rx_buffer.head + 1) % UART_BUFFER_SIZE;
    }

    irq_set_enabled(irq_num, false);
    rx_buffer.count -= available;
    irq_set_enabled(irq_num, true);

    *len = available;
    return true;
}

void uart_process_received_data(void) {
    if (!uart_initialized) return;

    extern void watchdog_update(void);

    // Process incoming UART data with priority-based routing:
    // 1. AT commands → Always processed (mode control and queries)
    // 2. HID commands (HID:RR:LLLL:...) → Always processed as HID injection
    // 3. Other data → Forwarded to CDC serial if Serial Mode enabled, otherwise ignored

    static char line_buffer[256];  // Increased for framed commands with markers
    static uint16_t line_pos = 0;  // Changed to uint16_t for larger buffer

    uint8_t temp_buffer[64];
    uint16_t temp_len = sizeof(temp_buffer);
    uint16_t bytes_processed = 0;

    while (uart_receive_data(temp_buffer, &temp_len) && temp_len > 0) {
        for (uint16_t i = 0; i < temp_len; i++) {
            uint8_t byte = temp_buffer[i];

            // ---------- SLIP state machine (priority over text parser) ----------
            if (byte == SLIP_END) {
                if (slip_in_frame) {
                    slip_dispatch();
                }
                slip_in_frame = true;
                slip_escape   = false;
                goto next_byte;
            }
            if (slip_in_frame) {
                if (byte == SLIP_ESC) {
                    slip_escape = true;
                } else {
                    uint8_t decoded;
                    if (slip_escape) {
                        decoded = (byte == SLIP_ESC_END) ? SLIP_END :
                                  (byte == SLIP_ESC_ESC) ? SLIP_ESC : byte;
                        slip_escape = false;
                    } else {
                        decoded = byte;
                    }
                    if (slip_len < SLIP_MAX_FRAME) {
                        slip_buf[slip_len++] = decoded;
                    } else {
                        // Frame too large — stay in SLIP mode and discard remaining
                        // bytes until the closing END. Do NOT fall through to text
                        // parser: binary frame data would misparse as AT commands.
                        slip_len    = 0;
                        slip_escape = false;
                        // slip_in_frame stays true; closing END will call
                        // slip_dispatch() with slip_len==0, which is a no-op.
                    }
                }
                goto next_byte;
            }

            // ---------- Text line parser (AT / HID / SERIAL commands) ----------
            {
                char ch = (char)byte;
                bool is_text = (ch >= 0x20 && ch <= 0x7E) || ch == '\r' || ch == '\n' || ch == '\t';

                if (is_text) {
                    if (ch == '\r' || ch == '\n') {
                        if (line_pos > 0) {
                            line_buffer[line_pos] = '\0';

                            if (strncmp(line_buffer, "AT", 2) == 0) {
                                process_at_command(line_buffer);
                                line_pos = 0;
                                goto next_byte;
                            }

                            if (strncmp(line_buffer, "SERIAL:", 7) == 0) {
                                const char* frame_data = line_buffer + 7;
                                uint16_t frame_len = line_pos - 7;
                                if (frame_len > 0 && device_status.serial_mode_enabled) {
                                    uart_to_cdc_enqueue((uint8_t*)frame_data, frame_len);
                                    uart_to_cdc_enqueue((uint8_t*)"\r\n", 2);
                                }
                                line_pos = 0;
                                goto next_byte;
                            }

                            uint8_t report_id;
                            uint8_t hid_data[64];
                            uint16_t hid_len;
                            if (parse_hid_command(line_buffer, &report_id, hid_data, &hid_len)) {
                                process_uart_hid_command(report_id, hid_data, hid_len);
                            }

                            line_pos = 0;
                        }
                    } else if (line_pos < sizeof(line_buffer) - 1) {
                        line_buffer[line_pos++] = ch;
                    }
                }
                // Non-text, non-SLIP bytes are ignored
            }

            next_byte:
            bytes_processed++;
            if (bytes_processed >= 64) {
                watchdog_update();
                bytes_processed = 0;
            }
        }

        temp_len = sizeof(temp_buffer);
    }
}