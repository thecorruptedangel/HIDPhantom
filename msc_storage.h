#ifndef MSC_STORAGE_H_
#define MSC_STORAGE_H_

#include <stdint.h>
#include <stdbool.h>

// MSC storage parameters
#define MSC_BLOCK_SIZE 512
#define MSC_BLOCK_COUNT 2048  // 1MB total

// Vendor/Product identification for MSC device
#define MSC_VENDOR_ID    "HID_PHN"
#define MSC_PRODUCT_ID   "PHANTOM"
#define MSC_PRODUCT_REV  "1.0"

// External reference to the generated disk image
extern const uint8_t msc_disk_image[];
extern const uint32_t msc_disk_sector_count;
extern const uint32_t msc_disk_sector_size;

#endif
