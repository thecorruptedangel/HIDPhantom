/*
 * LwIP Network Integration for USB RNDIS/ECM Network Bridge
 */

#include "net_lwip.h"
#include "uart_comm.h"
#include "tusb.h"
#include "pico/time.h"
#include "hardware/watchdog.h"

#include "networking/dhserver.h"
#include "networking/dnserver.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/ip.h"
#include "netif/ethernet.h"

#include <string.h>

#define INIT_IP4(a, b, c, d) { PP_HTONL(LWIP_MAKEU32(a, b, c, d)) }

// LwIP network interface
static struct netif netif_data;
static bool network_initialized = false;
static bool lwip_global_init_done = false;

// Network parameters (Pico IP: 192.168.4.100 to match ESP8266 subnet)
static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 4, 100);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

// DHCP database (must be in RAM to store assigned MAC addresses)
// Range 192.168.4.2-4.10 to avoid conflict with ESP8266's pool (.50+)
static dhcp_entry_t dhcp_entries[] = {
    /* mac, ip address, lease time (seconds) */
    {{0}, INIT_IP4(192, 168, 4, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 4, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 4, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router = INIT_IP4(192, 168, 4, 100),  // RP2040 acts as gateway/router
    .port = 67,                             // DHCP server port
    .dns = INIT_IP4(192, 168, 4, 100),      // DNS points to us
    .domain = NULL,                         // No DNS domain suffix
    .num_entry = TU_ARRAY_SIZE(dhcp_entries),
    .entries = dhcp_entries
};

// Forward declarations
static err_t linkoutput_fn(struct netif *netif, struct pbuf *p);
static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr);
static err_t netif_init_cb(struct netif *netif);
static bool dns_query_proc(const char *name, ip4_addr_t *addr);

// LwIP link output function - sends packets to USB
static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void)netif;

    // Update watchdog to prevent reset during network operations
    watchdog_update();

    for (;;) {
        // Check if TinyUSB is ready
        if (!tud_ready())
            return ERR_USE;

        // Check if network driver can accept another packet
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);
            return ERR_OK;
        }

        // Process TinyUSB tasks to finish transmitting prior packet
        tud_task();
    }
}

// IPv4 output function
static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

// Network interface initialization callback
static err_t netif_init_cb(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'u';  // USB network
    netif->name[1] = 's';
    netif->linkoutput = linkoutput_fn;
    netif->output = ip4_output_fn;

    return ERR_OK;
}

// DNS query handler
static bool dns_query_proc(const char *name, ip4_addr_t *addr) {
    (void)name;  // Unused - we resolve all queries to our IP
    // Resolve any DNS query to our IP address (captive portal-like)
    *addr = ipaddr;
    return true;
}

// Initialize network stack
bool net_init(void) {
    if (network_initialized) {
        return true;  // Already initialized
    }

    // Initialize LwIP globally only once (not per net_init)
    if (!lwip_global_init_done) {
        lwip_init();
        lwip_global_init_done = true;
    }

    // Clear netif structure to ensure clean state
    memset(&netif_data, 0, sizeof(netif_data));

    // Clear DHCP entries from previous sessions
    for (size_t i = 0; i < TU_ARRAY_SIZE(dhcp_entries); i++) {
        memset(dhcp_entries[i].mac, 0, 6);
    }

    // Setup network interface
    struct netif *netif = &netif_data;

    // Set MAC address (toggle LSB to differ from USB MAC)
    extern uint8_t tud_network_mac_address[6];
    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, tud_network_mac_address, 6);
    netif->hwaddr[5] ^= 0x01;

    // Add network interface
    netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ethernet_input);
    if (netif == NULL) {
        return false;
    }

    // Set as default interface
    netif_set_default(netif);
    netif_set_link_up(netif);
    netif_set_up(netif);

    // Initialize DHCP server
    if (dhserv_init(&dhcp_config) != ERR_OK) {
        netif_set_down(&netif_data);
        netif_set_link_down(&netif_data);
        netif_remove(&netif_data);
        memset(&netif_data, 0, sizeof(netif_data));
        return false;
    }

    // Initialize DNS server
    if (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK) {
        dhserv_free();
        netif_set_down(&netif_data);
        netif_set_link_down(&netif_data);
        netif_remove(&netif_data);
        memset(&netif_data, 0, sizeof(netif_data));
        return false;
    }

    network_initialized = true;
    return true;
}

// Deinitialize network stack
void net_deinit(void) {
    if (!network_initialized) {
        return;
    }

    // Free DNS server
    dnserv_free();

    // Free DHCP server
    dhserv_free();

    // Properly shut down network interface
    netif_set_down(&netif_data);
    netif_set_link_down(&netif_data);
    netif_set_default(NULL);

    // Remove network interface
    netif_remove(&netif_data);

    // Clear the structure to ensure clean state for next init
    memset(&netif_data, 0, sizeof(netif_data));

    network_initialized = false;
}

// Check if network is initialized
bool net_is_initialized(void) {
    return network_initialized;
}

// Process network timers
void net_task(void) {
    if (network_initialized) {
        sys_check_timeouts();
    }
}

//--------------------------------------------------------------------+
// Packet Forwarding (USB ↔ UART Bridge)
//--------------------------------------------------------------------+

/**
 * Forward packet from UART to USB network interface
 * Injects packets received from ESP8266 (via UART) into lwIP stack
 */
bool net_forward_from_uart(const uint8_t* packet, uint16_t len) {
    if (!network_initialized || !netif_is_up(&netif_data)) {
        return false;
    }

    if (len == 0 || packet == NULL) {
        return false;
    }

    struct netif *netif = &netif_data;

    // Allocate pbuf for the packet
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        return false;
    }

    // Copy packet data to pbuf
    pbuf_take(p, packet, len);

    // Inject into lwIP stack for transmission to USB host
    if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
        return false;
    }

    return true;
}

/**
 * Check if IP packet should be forwarded to UART (ESP8266 WiFi side)
 * Packets destined for 192.168.4.1 (ESP8266) or .50+ (WiFi clients) should be forwarded
 */
static bool should_forward_to_uart(const uint8_t* packet, uint16_t len) {
    // Need at least IP header (20 bytes minimum)
    if (len < 20) {
        return false;
    }

    // Check if netbridge mode is enabled
    connected_device_status_t* status = get_connected_device_status();
    if (status == NULL || !status->netbridge_mode_enabled) {
        return false;
    }

    // Extract destination IP from IP header (bytes 16-19)
    uint32_t dest_ip = (packet[16] << 24) | (packet[17] << 16) |
                       (packet[18] << 8) | packet[19];

    // Forward to UART if destination is:
    // - 192.168.4.1 (ESP8266 itself)
    // - 192.168.4.50+ (WiFi clients on ESP8266 AP)
    uint32_t subnet_base = (192 << 24) | (168 << 16) | (4 << 8);
    uint32_t esp8266_ip = subnet_base | 1;    // 192.168.4.1
    uint32_t wifi_client_start = subnet_base | 50;  // 192.168.4.50

    // Check if destination is ESP8266 or WiFi client range
    if (dest_ip == esp8266_ip || dest_ip >= wifi_client_start) {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------+
// TinyUSB Network Callbacks
//--------------------------------------------------------------------+

// Initialize callback - called when USB network device is initialized
void tud_network_init_cb(void) {
    // Network device is ready, initialization is done in net_init()
    // This callback is required by TinyUSB but we don't need to do anything here
}

// Receive callback - called when USB host sends data to device
bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    if (!network_initialized || !netif_is_up(&netif_data)) {
        // Network not ready - signal TinyUSB to renew buffer anyway
        tud_network_recv_renew();
        return false;
    }

    struct netif *netif = &netif_data;

    if (size) {
        // Check if packet should be forwarded to UART (ESP8266/WiFi side)
        if (should_forward_to_uart(src, size)) {
            // Forward to UART instead of processing locally
            uart_send_net_packet(src, size);
            tud_network_recv_renew();
            return true;
        }

        // Allocate pbuf for received data
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p == NULL) {
            tud_network_recv_renew();
            return false;
        }

        // Copy data to pbuf
        pbuf_take(p, src, size);

        // Pass to LwIP - only free pbuf on error
        if (netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
        }

        // Signal TinyUSB that frame has been processed
        tud_network_recv_renew();
    }

    return true;
}

// Transmit callback - called to copy data from pbuf to USB buffer
uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    if (!network_initialized) {
        return 0;
    }

    struct pbuf *p = (struct pbuf *)ref;
    (void)arg;

    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

//--------------------------------------------------------------------+
// LwIP System Functions
//--------------------------------------------------------------------+

// LwIP mutex protection (not needed for single-threaded NO_SYS mode)
sys_prot_t sys_arch_protect(void) {
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
}

// LwIP millisecond timer
uint32_t sys_now(void) {
    return to_ms_since_boot(get_absolute_time());
}
