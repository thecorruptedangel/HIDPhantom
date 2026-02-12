#include "tusb.h"
#include "usb_descriptors.h"
#include <string.h>
#include "pico/mutex.h"

volatile device_info_t connected_device_info = {0};
volatile bool use_device_info = false;
volatile bool device_reset_in_progress = false;
volatile descriptor_type_t current_descriptor_type = DESC_TYPE_DEFAULT;

report_queue_entry_t report_queue[USB_PROXY_QUEUE_SIZE];
volatile uint8_t report_queue_head = 0;
volatile uint8_t report_queue_tail = 0;

extern mutex_t queue_mutex;
extern mutex_t state_mutex;

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define USB_VID 0xCafe
#define USB_BCD 0x0200

static tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    mutex_enter_blocking(&state_mutex);

    // Network-only mode requires IAD device class and dual configurations (RNDIS + ECM)
    if (current_descriptor_type == DESC_TYPE_NET_ONLY) {
        desc_device.bDeviceClass = TUSB_CLASS_MISC;
        desc_device.bDeviceSubClass = MISC_SUBCLASS_COMMON;
        desc_device.bDeviceProtocol = MISC_PROTOCOL_IAD;
        desc_device.bNumConfigurations = 2;  // Support both RNDIS and ECM
    }
    // HID+NET composite uses single configuration, classes defined at interface level
    else if (current_descriptor_type == DESC_TYPE_HID_NET_COMPOSITE) {
        desc_device.bDeviceClass = TUSB_CLASS_MISC;
        desc_device.bDeviceSubClass = MISC_SUBCLASS_COMMON;
        desc_device.bDeviceProtocol = MISC_PROTOCOL_IAD;
        desc_device.bNumConfigurations = 1;  // Single config for HID+RNDIS composite
    }
    else if (use_device_info && connected_device_info.valid) {
        desc_device.idVendor = connected_device_info.vid;
        desc_device.idProduct = connected_device_info.pid;
        desc_device.bcdDevice = connected_device_info.bcdDevice;
        desc_device.bNumConfigurations = 1;

        if (connected_device_info.bDeviceClass != 0) {
            desc_device.bDeviceClass = connected_device_info.bDeviceClass;
            desc_device.bDeviceSubClass = connected_device_info.bDeviceSubClass;
            desc_device.bDeviceProtocol = connected_device_info.bDeviceProtocol;
        }
    } else {
        desc_device.bDeviceClass = 0x00;
        desc_device.bDeviceSubClass = 0x00;
        desc_device.bDeviceProtocol = 0x00;
        desc_device.bNumConfigurations = 1;
    }

    mutex_exit(&state_mutex);
    return (uint8_t const *) &desc_device;
}

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL
};

// Interface numbers for HID+CDC composite mode
enum {
    ITF_NUM_HID_CDC = 0,
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL_CDC
};

// Interface numbers for HID+MSC composite mode
enum {
    ITF_NUM_HID_MSC = 0,
    ITF_NUM_MSC,
    ITF_NUM_TOTAL_MSC
};

// Interface numbers for HID+CDC+MSC combined mode
enum {
    ITF_NUM_HID_CDC_MSC = 0,
    ITF_NUM_CDC_COMBINED,
    ITF_NUM_CDC_DATA_COMBINED,
    ITF_NUM_MSC_COMBINED,
    ITF_NUM_TOTAL_CDC_MSC
};

// Interface numbers for Network-only mode (NO HID)
enum {
    ITF_NUM_NET_ONLY = 0,
    ITF_NUM_NET_DATA_ONLY,
    ITF_NUM_TOTAL_NET_ONLY
};

// Interface numbers for HID+NET composite mode (HID and Network coexist)
enum {
    ITF_NUM_HID_NET = 0,
    ITF_NUM_NET_COMPOSITE,
    ITF_NUM_NET_DATA_COMPOSITE,
    ITF_NUM_TOTAL_HID_NET
};

#define EPNUM_HID 0x81
#define EPNUM_CDC_NOTIF 0x82
#define EPNUM_CDC_DATA 0x03
#define EPNUM_MSC_OUT 0x04
#define EPNUM_MSC_IN 0x84

// Network-only endpoints (NO HID when network mode is enabled)
#define EPNUM_NET_NOTIF 0x81
#define EPNUM_NET_OUT 0x02
#define EPNUM_NET_IN 0x82

// HID+NET composite endpoints (HID and Network share device)
#define EPNUM_HID_NET_HID 0x81        // HID endpoint
#define EPNUM_HID_NET_NOTIF 0x82      // Network notification endpoint
#define EPNUM_HID_NET_OUT 0x03        // Network OUT endpoint
#define EPNUM_HID_NET_IN 0x83         // Network IN endpoint

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define CONFIG_TOTAL_LEN_CDC (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)
#define CONFIG_TOTAL_LEN_MSC (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_MSC_DESC_LEN)
#define CONFIG_TOTAL_LEN_CDC_MSC (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN)

// Network-only configurations (NO HID) - dual config for RNDIS (config 0) and ECM (config 1)
#define CONFIG_TOTAL_LEN_NET_RNDIS (TUD_CONFIG_DESC_LEN + TUD_RNDIS_DESC_LEN)
#define CONFIG_TOTAL_LEN_NET_ECM (TUD_CONFIG_DESC_LEN + TUD_CDC_ECM_DESC_LEN)

// HID+NET composite configurations (HID + Network coexist)
#define CONFIG_TOTAL_LEN_HID_NET_RNDIS (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_RNDIS_DESC_LEN)
#define CONFIG_TOTAL_LEN_HID_NET_ECM (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_ECM_DESC_LEN)

// String descriptor indices
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC_INTERFACE,
    STRID_MSC_INTERFACE,
    STRID_NET_INTERFACE,
    STRID_MAC
};

// Different HID report descriptors for different device types
uint8_t const desc_hid_report_keyboard[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD))
};

uint8_t const desc_hid_report_mouse[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE))
};

uint8_t const desc_hid_report_composite[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
    TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(REPORT_ID_SYSTEM_CONTROL))
};

// Default to composite for backward compatibility
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
    TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(REPORT_ID_SYSTEM_CONTROL))
};

// Configuration descriptors for different device types
uint8_t const desc_fs_configuration_keyboard[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report_keyboard), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t const desc_fs_configuration_mouse[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_report_mouse), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t const desc_fs_configuration_composite[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

// Default configuration (composite)
uint8_t const desc_fs_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

// HID + CDC composite configuration for serial CDC mode
uint8_t const desc_fs_configuration_hid_cdc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_CDC, 0, CONFIG_TOTAL_LEN_CDC, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_CDC, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_DATA, EPNUM_CDC_DATA | 0x80, 64)
};

// HID + MSC composite configuration for storage mode
uint8_t const desc_fs_configuration_hid_msc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_MSC, 0, CONFIG_TOTAL_LEN_MSC, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_MSC, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)
};

// HID + CDC + MSC combined configuration for both serial and storage modes
uint8_t const desc_fs_configuration_hid_cdc_msc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_CDC_MSC, 0, CONFIG_TOTAL_LEN_CDC_MSC, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_CDC_MSC, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_COMBINED, STRID_CDC_INTERFACE, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_DATA, EPNUM_CDC_DATA | 0x80, 64),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC_COMBINED, STRID_MSC_INTERFACE, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)
};

// Network-only (RNDIS) configuration - Config 0 for Windows/Linux - NO HID
uint8_t const desc_fs_configuration_net_rndis[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_NET_ONLY, 0, CONFIG_TOTAL_LEN_NET_RNDIS, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_NET_ONLY, STRID_NET_INTERFACE, EPNUM_NET_NOTIF, 8, EPNUM_NET_OUT, EPNUM_NET_IN, CFG_TUD_NET_ENDPOINT_SIZE)
};

// Network-only (ECM) configuration - Config 1 for Linux/macOS - NO HID
uint8_t const desc_fs_configuration_net_ecm[] = {
    TUD_CONFIG_DESCRIPTOR(2, ITF_NUM_TOTAL_NET_ONLY, 0, CONFIG_TOTAL_LEN_NET_ECM, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_NET_ONLY, STRID_NET_INTERFACE, STRID_MAC, EPNUM_NET_NOTIF, 64, EPNUM_NET_OUT, EPNUM_NET_IN, CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU)
};

// HID+NET composite (RNDIS) configuration - Config 0 for Windows/Linux
uint8_t const desc_fs_configuration_hid_net_rndis[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_HID_NET, 0, CONFIG_TOTAL_LEN_HID_NET_RNDIS, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_NET, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID_NET_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_NET_COMPOSITE, STRID_NET_INTERFACE, EPNUM_HID_NET_NOTIF, 8, EPNUM_HID_NET_OUT, EPNUM_HID_NET_IN, CFG_TUD_NET_ENDPOINT_SIZE)
};

// HID+NET composite (ECM) configuration - Config 1 for Linux/macOS
uint8_t const desc_fs_configuration_hid_net_ecm[] = {
    TUD_CONFIG_DESCRIPTOR(2, ITF_NUM_TOTAL_HID_NET, 0, CONFIG_TOTAL_LEN_HID_NET_ECM, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_NET, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_composite), EPNUM_HID_NET_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_NET_COMPOSITE, STRID_NET_INTERFACE, STRID_MAC, EPNUM_HID_NET_NOTIF, 64, EPNUM_HID_NET_OUT, EPNUM_HID_NET_IN, CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    switch (current_descriptor_type) {
        case DESC_TYPE_KEYBOARD_ONLY:
            return desc_fs_configuration_keyboard;
        case DESC_TYPE_MOUSE_ONLY:
            return desc_fs_configuration_mouse;
        case DESC_TYPE_COMPOSITE:
            return desc_fs_configuration_composite;
        case DESC_TYPE_HID_CDC_COMPOSITE:
            return desc_fs_configuration_hid_cdc;
        case DESC_TYPE_HID_MSC_COMPOSITE:
            return desc_fs_configuration_hid_msc;
        case DESC_TYPE_HID_CDC_MSC_COMPOSITE:
            return desc_fs_configuration_hid_cdc_msc;

        // Network-only mode: index 0=RNDIS (Windows/Linux), index 1=ECM (Linux/macOS) - NO HID
        case DESC_TYPE_NET_ONLY:
            return (index == 0) ? desc_fs_configuration_net_rndis : desc_fs_configuration_net_ecm;

        // HID+NET composite mode: Only RNDIS config (single config for Windows compatibility)
        case DESC_TYPE_HID_NET_COMPOSITE:
            return desc_fs_configuration_hid_net_rndis;  // Single config: HID + RNDIS

        case DESC_TYPE_DEFAULT:
        default:
            return desc_fs_configuration;
    }
}

// Network MAC address (must be globally unique)
uint8_t tud_network_mac_address[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};

char const* default_string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // STRID_LANGID
    "Realtek",                       // STRID_MANUFACTURER
    "Realtek Network Controller",    // STRID_PRODUCT
    "123456789012",                  // STRID_SERIAL
    "CDC Serial Interface",          // STRID_CDC_INTERFACE
    "Mass Storage Interface",        // STRID_MSC_INTERFACE
    "Realtek USB GbE Family Controller", // STRID_NET_INTERFACE
    // STRID_MAC is handled separately
};

static uint16_t _desc_str[64];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;
    const char* str;

    // Handle MAC address string descriptor
    if (index == STRID_MAC) {
        // Convert MAC address to hex string in UTF-16
        for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++) {
            _desc_str[1 + i*2    ] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
            _desc_str[1 + i*2 + 1] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
        }
        chr_count = sizeof(tud_network_mac_address) * 2;
        _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
        return _desc_str;
    }

    mutex_enter_blocking(&state_mutex);

    if (use_device_info && connected_device_info.valid) {
        switch (index) {
            case STRID_MANUFACTURER:
                str = connected_device_info.manufacturer[0] ? (const char*)connected_device_info.manufacturer : default_string_desc_arr[STRID_MANUFACTURER];
                break;
            case STRID_PRODUCT:
                str = connected_device_info.product[0] ? (const char*)connected_device_info.product : default_string_desc_arr[STRID_PRODUCT];
                break;
            case STRID_SERIAL:
                str = connected_device_info.serial[0] ? (const char*)connected_device_info.serial : default_string_desc_arr[STRID_SERIAL];
                break;
            default:
                str = (index < sizeof(default_string_desc_arr)/sizeof(default_string_desc_arr[0])) ?
                      default_string_desc_arr[index] : NULL;
                break;
        }
    } else {
        str = (index < sizeof(default_string_desc_arr)/sizeof(default_string_desc_arr[0])) ?
              default_string_desc_arr[index] : NULL;
    }

    mutex_exit(&state_mutex);

    if (!str) return NULL;

    if (index == STRID_LANGID) {
        memcpy(&_desc_str[1], str, 2);
        chr_count = 1;
    } else {
        chr_count = strlen(str);
        if (chr_count > 63) chr_count = 63;

        for(uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
    return _desc_str;
}

uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf) {
    (void) itf;

    switch (current_descriptor_type) {
        case DESC_TYPE_KEYBOARD_ONLY:
            return desc_hid_report_keyboard;
        case DESC_TYPE_MOUSE_ONLY:
            return desc_hid_report_mouse;
        case DESC_TYPE_COMPOSITE:
            return desc_hid_report_composite;
        case DESC_TYPE_HID_CDC_COMPOSITE:
            return desc_hid_report_composite;  // Use composite HID report for serial CDC mode
        case DESC_TYPE_HID_MSC_COMPOSITE:
            return desc_hid_report_composite;  // Use composite HID report for storage mode
        case DESC_TYPE_HID_CDC_MSC_COMPOSITE:
            return desc_hid_report_composite;  // Use composite HID report for combined mode
        case DESC_TYPE_NET_ONLY:
            return NULL;  // NO HID interface in network-only mode
        case DESC_TYPE_HID_NET_COMPOSITE:
            return desc_hid_report_composite;  // Use composite HID report for HID+NET mode
        case DESC_TYPE_DEFAULT:
        default:
            return desc_hid_report;
    }
}

void update_device_info(uint16_t vid, uint16_t pid, const char* manufacturer, 
                       const char* product, const char* serial, uint16_t bcdDevice) {
    connected_device_info.vid = vid;
    connected_device_info.pid = pid;
    connected_device_info.bcdDevice = bcdDevice;
    
    if (manufacturer) {
        strncpy((char*)connected_device_info.manufacturer, manufacturer, MAX_STRING_LEN - 1);
        ((char*)connected_device_info.manufacturer)[MAX_STRING_LEN - 1] = '\0';
    } else {
        ((char*)connected_device_info.manufacturer)[0] = '\0';
    }
    
    if (product) {
        strncpy((char*)connected_device_info.product, product, MAX_STRING_LEN - 1);
        ((char*)connected_device_info.product)[MAX_STRING_LEN - 1] = '\0';
    } else {
        ((char*)connected_device_info.product)[0] = '\0';
    }
    
    if (serial) {
        strncpy((char*)connected_device_info.serial, serial, MAX_STRING_LEN - 1);
        ((char*)connected_device_info.serial)[MAX_STRING_LEN - 1] = '\0';
    } else {
        ((char*)connected_device_info.serial)[0] = '\0';
    }
    
    connected_device_info.valid = true;
    connected_device_info.last_update_ms = to_ms_since_boot(get_absolute_time());
}

void reset_to_default_descriptors(void) {
    connected_device_info.valid = false;
    connected_device_info.vid = 0;
    connected_device_info.pid = 0;
    connected_device_info.bcdDevice = 0x0100;
    connected_device_info.manufacturer[0] = '\0';
    connected_device_info.product[0] = '\0';
    connected_device_info.serial[0] = '\0';
    
    use_device_info = false;
    
    desc_device.idVendor = USB_VID;
    desc_device.idProduct = USB_PID;

    desc_device.bDeviceClass = 0x00;
    desc_device.bDeviceSubClass = 0x00;
    desc_device.bDeviceProtocol = 0x00;
}

void clear_report_queue(void) {
    report_queue_head = 0;
    report_queue_tail = 0;
    memset(report_queue, 0, sizeof(report_queue));
}

void reset_all_state(void) {
    reset_to_default_descriptors();
    clear_report_queue();
    device_reset_in_progress = false;
}

bool queue_report(const report_queue_entry_t* entry) {
    if (entry == NULL) return false;
    
    if (device_reset_in_progress) return false;
    
    uint8_t next_head = (report_queue_head + 1) % USB_PROXY_QUEUE_SIZE;
    
    if (next_head != report_queue_tail) {
        memcpy(&report_queue[report_queue_head], entry, sizeof(report_queue_entry_t));
        report_queue_head = next_head;
        return true;
    }
    
    return false;
}

bool dequeue_report(report_queue_entry_t* entry) {
    if (entry == NULL) return false;
    
    if (report_queue_head != report_queue_tail) {
        memcpy(entry, &report_queue[report_queue_tail], sizeof(report_queue_entry_t));
        report_queue_tail = (report_queue_tail + 1) % USB_PROXY_QUEUE_SIZE;
        return true;
    }
    
    return false;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
}

void set_descriptor_type(descriptor_type_t type) {
    current_descriptor_type = type;
}