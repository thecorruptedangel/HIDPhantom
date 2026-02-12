/*
 * LwIP Network Integration for USB RNDIS/ECM Network Bridge
 *
 * Provides network stack initialization, DHCP server, and DNS server
 * for USB virtual network adapter functionality.
 */

#ifndef NET_LWIP_H_
#define NET_LWIP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the network stack (LwIP, DHCP server, DNS server)
 * Sets up the Pico as IP 192.168.4.100 with DHCP pool 192.168.4.2-4.4
 * Advertises itself as gateway/router to enable packet forwarding
 *
 * @return true on success, false on failure
 */
bool net_init(void);

/**
 * Deinitialize the network stack and free resources
 */
void net_deinit(void);

/**
 * Check if network stack is initialized and running
 *
 * @return true if network is active, false otherwise
 */
bool net_is_initialized(void);

/**
 * Process LwIP timers and tasks
 * Must be called regularly from main loop (at least every 100ms)
 */
void net_task(void);

/**
 * Forward a packet received from UART to the USB network interface
 * This function injects IP packets into the lwIP stack for transmission to USB host
 *
 * @param packet Pointer to IP packet data
 * @param len Length of the packet in bytes
 * @return true if packet was successfully queued, false on error
 */
bool net_forward_from_uart(const uint8_t* packet, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* NET_LWIP_H_ */
