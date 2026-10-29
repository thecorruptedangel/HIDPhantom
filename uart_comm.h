#ifndef UART_COMM_H_
#define UART_COMM_H_

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "tusb.h"

#define UART_ID uart0
#define BAUD_RATE 921600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
// For xiao 
#define UART_TX_PIN 28
#define UART_RX_PIN 29

// //for pico
// #define UART_TX_PIN 4
// #define UART_RX_PIN 5

// Ring buffer large enough to buffer a full SLIP frame (1514 bytes) between polls
#define UART_BUFFER_SIZE 4096

// SLIP framing constants (RFC 1055)
#define SLIP_END      0xC0u
#define SLIP_ESC      0xDBu
#define SLIP_ESC_END  0xDCu
#define SLIP_ESC_ESC  0xDDu
#define SLIP_MAX_FRAME 1514u  // Max Ethernet frame size

typedef struct {
    uint8_t buffer[UART_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} uart_buffer_t;

// Unified UART TX queue — all outgoing UART traffic serialized here.
// Sized for 2 max SLIP frames (worst-case fully escaped: 1514*2+2 = 3030 bytes each)
// plus headroom for concurrent AT responses and HID intercept copies.
#define UART_TX_QUEUE_SIZE 8192

typedef struct {
    uint8_t buffer[UART_TX_QUEUE_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} uart_tx_queue_t;

// CDC to UART queue for non-blocking forwarding
#define CDC_TO_UART_QUEUE_SIZE 4096

typedef struct {
    uint8_t buffer[CDC_TO_UART_QUEUE_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} cdc_to_uart_queue_t;

// UART to CDC queue for non-blocking forwarding (ESP8266 → Host PC)
#define UART_TO_CDC_QUEUE_SIZE 4096

typedef struct {
    uint8_t buffer[UART_TO_CDC_QUEUE_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} uart_to_cdc_queue_t;

typedef enum {
    DEVICE_TYPE_NONE = 0,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_COMPOSITE,
    DEVICE_TYPE_UNKNOWN
} device_type_t;

typedef struct {
    device_type_t type;
    uint16_t vid;
    uint16_t pid;
    char manufacturer[64];
    char product[64];
    char serial[64];
    uint8_t country_code;  // Keyboard layout country code (0=US, 1=Arabic, 2=Belgian, etc.)
    bool connected;
    bool hybrid_mode_enabled;  // Hybrid mode flag
    device_type_t actual_device_type;  // Store actual hardware type when in hybrid mode
    bool serial_mode_enabled;  // Serial CDC mode flag - enables CDC serial interface
    bool storage_mode_enabled;  // Storage mode flag - enables MSC device
    bool netbridge_mode_enabled;  // Network bridge mode flag - enables USB RNDIS/ECM network interface
} connected_device_status_t;

void uart_comm_init(void);
void uart_send_hid_report(uint8_t report_id, const uint8_t* data, uint16_t len);
void uart_send_response(const char* response);
bool uart_receive_data(uint8_t* buffer, uint16_t* len);
void uart_process_received_data(void);
void uart_send_device_status_update(void);
void uart_drain_tx_queue(void);
connected_device_status_t* get_connected_device_status(void);

// Network packet forwarding over UART
void uart_send_net_packet(const uint8_t* packet, uint16_t len);
typedef void (*net_packet_callback_t)(const uint8_t* packet, uint16_t len);
void uart_set_net_packet_callback(net_packet_callback_t callback);

// CDC to UART queue management functions
void cdc_to_uart_queue_init(void);
bool cdc_to_uart_enqueue(const uint8_t* data, uint16_t len);
uint16_t cdc_to_uart_get_free_space(void);
void process_cdc_to_uart_queue(void);

// UART to CDC queue management functions
void uart_to_cdc_queue_init(void);
bool uart_to_cdc_enqueue(const uint8_t* data, uint16_t len);
uint16_t uart_to_cdc_get_free_space(void);
void process_uart_to_cdc_queue(void);

#endif