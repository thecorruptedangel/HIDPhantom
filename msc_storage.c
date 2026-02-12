#include "msc_storage.h"
#include "tusb.h"
#include <string.h>

// These will be defined in the auto-generated storage_data.c file
extern const uint8_t msc_disk_image[];
extern const uint32_t msc_disk_sector_count;
extern const uint32_t msc_disk_sector_size;

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;

    const char vid[] = MSC_VENDOR_ID;
    const char pid[] = MSC_PRODUCT_ID;
    const char rev[] = MSC_PRODUCT_REV;

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command
// Return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    return true; // Always ready (embedded disk)
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void) lun;

    *block_count = MSC_BLOCK_COUNT;
    *block_size = MSC_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun;
    (void) power_condition;
    (void) start;
    (void) load_eject;

    return true;
}

// Callback invoked when received READ10 command
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void) lun;

    // Check if read is within bounds
    if (lba >= MSC_BLOCK_COUNT) {
        return -1; // Error: out of bounds
    }

    // Calculate byte offset in disk image
    uint32_t byte_offset = lba * MSC_BLOCK_SIZE + offset;

    // Ensure we don't read past the end of the disk
    uint32_t total_size = MSC_BLOCK_COUNT * MSC_BLOCK_SIZE;
    if (byte_offset + bufsize > total_size) {
        bufsize = total_size - byte_offset;
    }

    // Copy data from embedded disk image
    memcpy(buffer, &msc_disk_image[byte_offset], bufsize);

    return (int32_t)bufsize;
}

// Callback invoked when received WRITE10 command
// Process data in buffer to disk's storage and return number of written bytes
// This is a read-only implementation, so we ignore writes but return success for compatibility
bool tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return false; // Read-only disk
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void) lun;
    (void) lba;
    (void) offset;
    (void) buffer;

    // Ignore writes (read-only), but return success for compatibility
    return (int32_t)bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 have their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    (void)bufsize; // Unused parameter
    void const* response = NULL;
    int32_t resplen = 0;

    // Most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // Negative means error -> stall and return error to host
            resplen = -1;
            break;
    }

    // Return response length
    if (resplen > 0) {
        if (in_xfer) {
            memcpy(buffer, response, (size_t)resplen);
        } else {
            // SCSI output
        }
    }

    return (int32_t)resplen;
}
