/*
 * RNDIS Definitions for USB Ethernet/RNDIS Adapter
 * Based on Microsoft's Remote NDIS specification
 */

#ifndef _RNDIS_PROTOCOL_H
#define _RNDIS_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RNDIS Protocol Version */
#define RNDIS_MAJOR_VERSION 1
#define RNDIS_MINOR_VERSION 0

/* Message Types */
#define REMOTE_NDIS_PACKET_MSG          0x00000001
#define REMOTE_NDIS_INITIALIZE_MSG      0x00000002
#define REMOTE_NDIS_HALT_MSG            0x00000003
#define REMOTE_NDIS_QUERY_MSG           0x00000004
#define REMOTE_NDIS_SET_MSG             0x00000005
#define REMOTE_NDIS_RESET_MSG           0x00000006
#define REMOTE_NDIS_INDICATE_STATUS_MSG 0x00000007
#define REMOTE_NDIS_KEEPALIVE_MSG       0x00000008
#define REMOTE_NDIS_INITIALIZE_CMPLT    0x80000002
#define REMOTE_NDIS_QUERY_CMPLT         0x80000004
#define REMOTE_NDIS_SET_CMPLT           0x80000005
#define REMOTE_NDIS_RESET_CMPLT         0x80000006
#define REMOTE_NDIS_KEEPALIVE_CMPLT     0x80000008

/* Device Flags */
#define RNDIS_DF_CONNECTIONLESS         0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED    0x00000002

/* Medium Types */
#define RNDIS_MEDIUM_802_3              0x00000000

/* Status Values */
#define RNDIS_STATUS_SUCCESS            0x00000000
#define RNDIS_STATUS_FAILURE            0xC0000001
#define RNDIS_STATUS_INVALID_DATA       0xC0010015
#define RNDIS_STATUS_NOT_SUPPORTED      0xC00000BB
#define RNDIS_STATUS_MEDIA_CONNECT      0x4001000B
#define RNDIS_STATUS_MEDIA_DISCONNECT   0x4001000C

/* NDIS Medium Types */
#define NDIS_MEDIUM_802_3               0x00000000

/* NDIS Media States */
#define NDIS_MEDIA_STATE_CONNECTED      0x00000000
#define NDIS_MEDIA_STATE_DISCONNECTED   0x00000001

/* NDIS MAC Options */
#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA 0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED  0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK         0x00000008

/* Object Identifiers (OIDs) - General */
#define OID_GEN_SUPPORTED_LIST          0x00010101
#define OID_GEN_HARDWARE_STATUS         0x00010102
#define OID_GEN_MEDIA_SUPPORTED         0x00010103
#define OID_GEN_MEDIA_IN_USE            0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD       0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE      0x00010106
#define OID_GEN_LINK_SPEED              0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE   0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE    0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE     0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE      0x0001010B
#define OID_GEN_VENDOR_ID               0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION      0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER   0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD       0x0001010F
#define OID_GEN_DRIVER_VERSION          0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE      0x00010111
#define OID_GEN_PROTOCOL_OPTIONS        0x00010112
#define OID_GEN_MAC_OPTIONS             0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS    0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS    0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION   0x00010116
#define OID_GEN_PHYSICAL_MEDIUM         0x00010202

/* OIDs - 802.3 Ethernet */
#define OID_802_3_PERMANENT_ADDRESS     0x01010101
#define OID_802_3_CURRENT_ADDRESS       0x01010102
#define OID_802_3_MULTICAST_LIST        0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE     0x01010104
#define OID_802_3_MAC_OPTIONS           0x01010105
#define OID_802_3_RCV_ERROR_ALIGNMENT   0x01020101
#define OID_802_3_XMIT_ONE_COLLISION    0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS  0x01020103

/* OIDs - Statistics */
#define OID_GEN_XMIT_OK                 0x00020101
#define OID_GEN_RCV_OK                  0x00020102
#define OID_GEN_XMIT_ERROR              0x00020103
#define OID_GEN_RCV_ERROR               0x00020104
#define OID_GEN_RCV_NO_BUFFER           0x00020105

/* OIDs - Power Management */
#define OID_PNP_CAPABILITIES            0xFD010100
#define OID_PNP_SET_POWER               0xFD010101
#define OID_PNP_QUERY_POWER             0xFD010102
#define OID_PNP_ADD_WAKE_UP_PATTERN     0xFD010103
#define OID_PNP_REMOVE_WAKE_UP_PATTERN  0xFD010104
#define OID_PNP_ENABLE_WAKE_UP          0xFD010106

/* OIDs - RNDIS Specific */
#define OID_GEN_RNDIS_CONFIG_PARAMETER  0x0001021B

/* RNDIS State */
typedef enum {
    rndis_uninitialized = 0,
    rndis_initialized,
    rndis_data_initialized
} rndis_state_t;

/* OID Type */
typedef uint32_t rndis_Oid_t;

/* USB Ethernet Statistics */
typedef struct {
    uint32_t txok;
    uint32_t rxok;
    uint32_t txbad;
    uint32_t rxbad;
} usb_eth_stat_t;

/* Generic RNDIS Message */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
} rndis_generic_msg_t;

/* RNDIS Initialize Message */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t MaxTransferSize;
} rndis_initialize_msg_t;

/* RNDIS Initialize Complete */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t DeviceFlags;
    uint32_t Medium;
    uint32_t MaxPacketsPerTransfer;
    uint32_t MaxTransferSize;
    uint32_t PacketAlignmentFactor;
    uint32_t AfListOffset;
    uint32_t AfListSize;
} rndis_initialize_cmplt_t;

/* RNDIS Query Message */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Oid;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t DeviceVcHandle;
} rndis_query_msg_t;

/* RNDIS Query Complete */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
} rndis_query_cmplt_t;

/* RNDIS Set Message */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Oid;
    uint32_t InformationBufferLength;
    uint32_t InformationBufferOffset;
    uint32_t DeviceVcHandle;
} rndis_set_msg_t;

/* RNDIS Set Complete */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
} rndis_set_cmplt_t;

/* RNDIS Reset Complete */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t Status;
    uint32_t AddressingReset;
} rndis_reset_cmplt_t;

/* RNDIS Keepalive Message */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
} rndis_keepalive_msg_t;

/* RNDIS Keepalive Complete */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t RequestId;
    uint32_t Status;
} rndis_keepalive_cmplt_t;

/* RNDIS Data Packet */
typedef struct {
    uint32_t MessageType;
    uint32_t MessageLength;
    uint32_t DataOffset;
    uint32_t DataLength;
    uint32_t OOBDataOffset;
    uint32_t OOBDataLength;
    uint32_t NumOOBDataElements;
    uint32_t PerPacketInfoOffset;
    uint32_t PerPacketInfoLength;
    uint32_t VcHandle;
    uint32_t Reserved;
} rndis_data_packet_t;

/* RNDIS Config Parameter */
typedef struct {
    uint32_t ParameterNameOffset;
    uint32_t ParameterNameLength;
    uint32_t ParameterType;
    uint32_t ParameterValueOffset;
    uint32_t ParameterValueLength;
} rndis_config_parameter_t;

/* Network driver report function (implemented by TinyUSB) */
extern void netd_report(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* _RNDIS_PROTOCOL_H */
