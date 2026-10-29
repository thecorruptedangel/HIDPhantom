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

static struct netif netif_data;
static bool network_initialized    = false;
static bool lwip_global_init_done  = false;

// Deferred receive: packet buffered in callback, processed from main loop
static struct pbuf *received_frame = NULL;

// Pico IP: 192.168.4.100 — acts as gateway for the USB host
static const ip4_addr_t ipaddr  = INIT_IP4(192, 168, 4, 100);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255,   0);
static const ip4_addr_t gateway = INIT_IP4(192, 168, 4,   100);

static dhcp_entry_t dhcp_entries[] = {
    {{0}, INIT_IP4(192, 168, 4, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 4, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 4, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router   = INIT_IP4(192, 168, 4, 100),
    .port     = 67,
    .dns      = INIT_IP4(192, 168, 4, 100),
    .domain   = NULL,
    .num_entry = TU_ARRAY_SIZE(dhcp_entries),
    .entries  = dhcp_entries
};

// ---- LwIP link output: send pbuf to USB host via RNDIS ----
static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void)netif;
    // Bounded retry: at 921600 baud the prior packet drains in < 2ms;
    // 200 iterations with a tud_task() each gives ~200ms headroom before
    // giving up. An unbound loop would stall Core 0 until the watchdog fires.
    watchdog_update();
    for (int i = 0; i < 200; i++) {
        if (!tud_ready()) return ERR_USE;
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);
            return ERR_OK;
        }
        tud_task();
    }
    return ERR_TIMEOUT;
}

// ---- netif init callback ----
static err_t netif_init_cb(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu        = CFG_TUD_NET_MTU;
    netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                        NETIF_FLAG_ETHERNET  | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state      = NULL;
    netif->name[0]    = 'u';
    netif->name[1]    = 's';
    netif->linkoutput = linkoutput_fn;
    netif->output     = etharp_output;  // direct — no wrapper needed
    return ERR_OK;
}

// ---- DNS: resolve everything to our IP (captive-portal style) ----
static bool dns_query_proc(const char *name, ip4_addr_t *addr) {
    (void)name;
    *addr = ipaddr;
    return true;
}

// ---- Init ----
bool net_init(void) {
    if (network_initialized) return true;

    if (!lwip_global_init_done) {
        lwip_init();
        lwip_global_init_done = true;
    }

    memset(&netif_data, 0, sizeof(netif_data));
    for (size_t i = 0; i < TU_ARRAY_SIZE(dhcp_entries); i++)
        memset(dhcp_entries[i].mac, 0, 6);

    extern uint8_t tud_network_mac_address[6];
    netif_data.hwaddr_len = 6;
    memcpy(netif_data.hwaddr, tud_network_mac_address, 6);
    netif_data.hwaddr[5] ^= 0x01;  // differ from USB-side MAC

    struct netif *netif = netif_add(&netif_data, &ipaddr, &netmask, &gateway,
                                    NULL, netif_init_cb, ethernet_input);
    if (!netif) return false;

    netif_set_default(netif);
    netif_set_link_up(netif);
    netif_set_up(netif);

    if (dhserv_init(&dhcp_config) != ERR_OK) goto fail_dhcp;
    if (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK) goto fail_dns;

    network_initialized = true;
    return true;

fail_dns:
    dhserv_free();
fail_dhcp:
    netif_set_down(&netif_data);
    netif_set_link_down(&netif_data);
    netif_remove(&netif_data);
    memset(&netif_data, 0, sizeof(netif_data));
    return false;
}

// ---- Deinit ----
void net_deinit(void) {
    if (!network_initialized) return;
    dnserv_free();
    dhserv_free();
    netif_set_down(&netif_data);
    netif_set_link_down(&netif_data);
    netif_set_default(NULL);
    netif_remove(&netif_data);
    memset(&netif_data, 0, sizeof(netif_data));
    if (received_frame) { pbuf_free(received_frame); received_frame = NULL; }
    network_initialized = false;
}

bool net_is_initialized(void) { return network_initialized; }

// ---- Main-loop tick: process deferred received frame + lwIP timers ----
void net_task(void) {
    if (!network_initialized) return;

    // Deferred receive: safe to call lwIP here (main-loop context, not USB callback)
    if (received_frame) {
        struct pbuf *p  = received_frame;
        received_frame  = NULL;
        if (netif_data.input(p, &netif_data) != ERR_OK)
            pbuf_free(p);
        tud_network_recv_renew();
    }

    sys_check_timeouts();
}

//--------------------------------------------------------------------+
// Packet Forwarding: UART (ESP8266 WiFi) → USB host
//--------------------------------------------------------------------+

// Packets from the ESP8266 arrive as full Ethernet frames.
// Transmit directly to USB host via RNDIS — do NOT use netif->input (that's the RX path).
bool net_forward_from_uart(const uint8_t *packet, uint16_t len) {
    if (!network_initialized || !tud_ready()) return false;
    if (!packet || len == 0) return false;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (!p) return false;
    pbuf_take(p, packet, len);

    bool sent = false;
    for (int i = 0; i < 10; i++) {
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);  // tud_network_xmit_cb copies synchronously
            sent = true;
            break;
        }
        tud_task();
    }

    pbuf_free(p);  // safe: xmit_cb already copied to USB buffer
    return sent;
}

// Packets from USB host: decide whether to route locally (lwIP) or forward to ESP8266
static bool should_forward_to_uart(const uint8_t *packet, uint16_t len) {
    // Ethernet frame = 14-byte header + IP header (≥20 bytes)
    if (len < 34) return false;

    connected_device_status_t *status = get_connected_device_status();
    if (!status || !status->netbridge_mode_enabled) return false;

    // Verify IPv4 EtherType (bytes 12-13)
    if (packet[12] != 0x08 || packet[13] != 0x00) return false;

    // Destination IP is at Ethernet offset 30 (14 Eth header + 16 bytes into IP header)
    uint32_t dest_ip = ((uint32_t)packet[30] << 24) | ((uint32_t)packet[31] << 16) |
                       ((uint32_t)packet[32] <<  8) |  (uint32_t)packet[33];

    uint32_t subnet_base      = (192UL << 24) | (168UL << 16) | (4UL << 8);
    uint32_t esp8266_ip       = subnet_base | 1;   // 192.168.4.1
    uint32_t wifi_client_base = subnet_base | 50;  // 192.168.4.50+

    return (dest_ip == esp8266_ip || dest_ip >= wifi_client_base);
}

//--------------------------------------------------------------------+
// TinyUSB Network Callbacks
//--------------------------------------------------------------------+

void tud_network_init_cb(void) {
    // Nothing to do — net_init() handles initialization
}

// Called from USB task context — only buffer the frame, do NOT call lwIP here
bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    if (!network_initialized || !netif_is_up(&netif_data)) {
        tud_network_recv_renew();
        return false;
    }

    if (size == 0) {
        tud_network_recv_renew();
        return true;
    }

    // Forward to ESP8266 if destined for WiFi side
    if (should_forward_to_uart(src, size)) {
        uart_send_net_packet(src, size);
        tud_network_recv_renew();
        return true;
    }

    // Drop if previous frame not yet consumed (flow control)
    if (received_frame) {
        tud_network_recv_renew();
        return false;
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    if (!p) {
        tud_network_recv_renew();
        return false;
    }
    pbuf_take(p, src, size);
    received_frame = p;
    // tud_network_recv_renew() called from net_task() after processing
    return true;
}

// Called by TinyUSB synchronously inside tud_network_xmit() to copy pbuf → USB buffer
uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    (void)arg;
    struct pbuf *p = (struct pbuf *)ref;
    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

//--------------------------------------------------------------------+
// LwIP System Glue (NO_SYS mode)
//--------------------------------------------------------------------+

sys_prot_t sys_arch_protect(void)          { return 0; }
void       sys_arch_unprotect(sys_prot_t p){ (void)p;  }
uint32_t   sys_now(void) { return to_ms_since_boot(get_absolute_time()); }
