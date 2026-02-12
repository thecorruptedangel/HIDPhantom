#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include <stdint.h>
#include <stdbool.h>

enum {
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE,
    REPORT_ID_CONSUMER_CONTROL,
    REPORT_ID_GAMEPAD,
    REPORT_ID_SYSTEM_CONTROL,
    REPORT_ID_COUNT
};

#define USB_PROXY_QUEUE_SIZE 32
#define MAX_STRING_LEN 64

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t bcdDevice;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    char manufacturer[MAX_STRING_LEN];
    char product[MAX_STRING_LEN];
    char serial[MAX_STRING_LEN];
    uint8_t configuration_value;
    bool valid;
    bool needs_reset;
    uint32_t last_update_ms;
} device_info_t;

typedef struct {
    uint8_t dev_addr;
    uint8_t instance;
    uint8_t interface_protocol;
    uint8_t report[64];
    uint16_t report_len;
} report_queue_entry_t;

extern volatile device_info_t connected_device_info;
extern volatile bool use_device_info;
extern volatile bool device_reset_in_progress;

// Dynamic descriptor support
typedef enum {
    DESC_TYPE_DEFAULT = 0,
    DESC_TYPE_KEYBOARD_ONLY,
    DESC_TYPE_MOUSE_ONLY,
    DESC_TYPE_COMPOSITE,
    DESC_TYPE_HID_CDC_COMPOSITE,          // HID + CDC Serial for serial CDC mode
    DESC_TYPE_HID_MSC_COMPOSITE,          // HID + MSC for storage mode
    DESC_TYPE_HID_CDC_MSC_COMPOSITE,      // HID + CDC + MSC (all modes enabled)
    DESC_TYPE_NET_ONLY,                   // Network adapter only (RNDIS/ECM) - NO HID
    DESC_TYPE_HID_NET_COMPOSITE,          // HID + Network (RNDIS/ECM) - coexist mode
} descriptor_type_t;

extern volatile descriptor_type_t current_descriptor_type;
void set_descriptor_type(descriptor_type_t type);

extern report_queue_entry_t report_queue[USB_PROXY_QUEUE_SIZE];
extern volatile uint8_t report_queue_head;
extern volatile uint8_t report_queue_tail;

void update_device_info(uint16_t vid, uint16_t pid, const char* manufacturer, 
                       const char* product, const char* serial, uint16_t bcdDevice);
void reset_to_default_descriptors(void);
void reset_all_state(void);
bool queue_report(const report_queue_entry_t* entry);
bool dequeue_report(report_queue_entry_t* entry);
void clear_report_queue(void);

#endif