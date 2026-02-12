#!/usr/bin/env python3
"""
FAT12 Disk Image Generator for HID Phantom Storage Mode

This script generates a 1MB FAT12 filesystem image containing files from
the storage_files/ directory. The generated image is embedded into the
RP2040 firmware as a const uint8_t array.

Usage: python3 gen_fat12_image.py <input_dir> <output_c_file>
"""

import os
import sys
import struct
import datetime
from pathlib import Path

# Disk parameters for 1MB FAT12 filesystem
SECTOR_SIZE = 512
TOTAL_SECTORS = 2048  # 1MB = 2048 * 512 bytes
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS = 1
NUM_FATS = 2
ROOT_DIR_ENTRIES = 224
SECTORS_PER_FAT = 9
ROOT_DIR_SECTORS = (ROOT_DIR_ENTRIES * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE

# Filesystem layout
BOOT_SECTOR = 0
FAT1_SECTOR = RESERVED_SECTORS
FAT2_SECTOR = FAT1_SECTOR + SECTORS_PER_FAT
ROOT_DIR_SECTOR = FAT2_SECTOR + SECTORS_PER_FAT
DATA_SECTOR = ROOT_DIR_SECTOR + ROOT_DIR_SECTORS

# Calculate available data clusters
DATA_SECTORS = TOTAL_SECTORS - DATA_SECTOR
NUM_CLUSTERS = DATA_SECTORS // SECTORS_PER_CLUSTER


def create_boot_sector(volume_label="PHANTOM"):
    """Create FAT12 boot sector with BPB (BIOS Parameter Block)"""
    boot_sector = bytearray(SECTOR_SIZE)

    # Jump instruction (3 bytes)
    boot_sector[0:3] = b'\xEB\x3C\x90'

    # OEM Name (8 bytes)
    boot_sector[3:11] = b'MSWIN4.1'

    # BIOS Parameter Block (BPB)
    struct.pack_into('<H', boot_sector, 11, SECTOR_SIZE)           # Bytes per sector
    struct.pack_into('B', boot_sector, 13, SECTORS_PER_CLUSTER)    # Sectors per cluster
    struct.pack_into('<H', boot_sector, 14, RESERVED_SECTORS)      # Reserved sectors
    struct.pack_into('B', boot_sector, 16, NUM_FATS)               # Number of FATs
    struct.pack_into('<H', boot_sector, 17, ROOT_DIR_ENTRIES)      # Root directory entries
    struct.pack_into('<H', boot_sector, 19, TOTAL_SECTORS)         # Total sectors (small)
    struct.pack_into('B', boot_sector, 21, 0xF8)                   # Media descriptor (fixed disk)
    struct.pack_into('<H', boot_sector, 22, SECTORS_PER_FAT)       # Sectors per FAT
    struct.pack_into('<H', boot_sector, 24, 63)                    # Sectors per track
    struct.pack_into('<H', boot_sector, 26, 255)                   # Number of heads
    struct.pack_into('<I', boot_sector, 28, 0)                     # Hidden sectors
    struct.pack_into('<I', boot_sector, 32, 0)                     # Total sectors (large)

    # Extended BPB (FAT12/16)
    struct.pack_into('B', boot_sector, 36, 0x80)                   # Drive number
    struct.pack_into('B', boot_sector, 37, 0)                      # Reserved
    struct.pack_into('B', boot_sector, 38, 0x29)                   # Extended boot signature
    struct.pack_into('<I', boot_sector, 39, 0x12345678)            # Volume serial number

    # Volume label (11 bytes, padded with spaces)
    label = volume_label[:11].ljust(11).encode('ascii')
    boot_sector[43:54] = label

    # Filesystem type (8 bytes)
    boot_sector[54:62] = b'FAT12   '

    # Boot signature
    boot_sector[510:512] = b'\x55\xAA'

    return bytes(boot_sector)


def create_empty_fat():
    """Create empty FAT12 table with media descriptor"""
    fat = bytearray(SECTORS_PER_FAT * SECTOR_SIZE)
    # First two entries: media descriptor
    fat[0] = 0xF8
    fat[1] = 0xFF
    fat[2] = 0xFF
    return bytes(fat)


def write_fat_entry(fat, cluster, value):
    """Write a FAT12 entry (12-bit value)"""
    fat = bytearray(fat)
    offset = cluster + (cluster // 2)  # Each entry is 1.5 bytes

    if cluster % 2 == 0:
        # Even cluster: lower 12 bits
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)
    else:
        # Odd cluster: upper 12 bits
        fat[offset] = (fat[offset] & 0x0F) | ((value << 4) & 0xF0)
        fat[offset + 1] = (value >> 4) & 0xFF

    return bytes(fat)


def create_directory_entry(filename, extension, attributes, first_cluster, file_size, timestamp=None):
    """Create a 32-byte directory entry"""
    entry = bytearray(32)

    # Filename (8 bytes, padded with spaces)
    entry[0:8] = filename[:8].upper().ljust(8).encode('ascii')

    # Extension (3 bytes, padded with spaces)
    entry[8:11] = extension[:3].upper().ljust(3).encode('ascii')

    # Attributes
    entry[11] = attributes

    # Reserved
    entry[12] = 0

    # Creation time/date (use current time if not provided)
    if timestamp is None:
        timestamp = datetime.datetime.now()

    # Time format: hhhhhmmmmmmsssss (2-second resolution)
    time_val = ((timestamp.hour << 11) | (timestamp.minute << 5) | (timestamp.second // 2))
    struct.pack_into('<H', entry, 14, time_val)

    # Date format: yyyyyyymmmmddddd (year relative to 1980)
    date_val = (((timestamp.year - 1980) << 9) | (timestamp.month << 5) | timestamp.day)
    struct.pack_into('<H', entry, 16, date_val)

    # Last access date
    struct.pack_into('<H', entry, 18, date_val)

    # High 16 bits of first cluster (FAT32, always 0 for FAT12)
    struct.pack_into('<H', entry, 20, 0)

    # Last modification time/date
    struct.pack_into('<H', entry, 22, time_val)
    struct.pack_into('<H', entry, 24, date_val)

    # First cluster (low 16 bits)
    struct.pack_into('<H', entry, 26, first_cluster)

    # File size
    struct.pack_into('<I', entry, 28, file_size)

    return bytes(entry)


def create_volume_label_entry(label):
    """Create volume label directory entry"""
    return create_directory_entry(label, "", 0x08, 0, 0)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_dir> <output_c_file>")
        sys.exit(1)

    input_dir = Path(sys.argv[1])
    output_file = Path(sys.argv[2])

    if not input_dir.exists():
        print(f"Error: Input directory '{input_dir}' does not exist")
        sys.exit(1)

    # Create empty disk image
    disk_image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)

    # Create boot sector
    boot_sector = create_boot_sector()
    disk_image[0:SECTOR_SIZE] = boot_sector

    # Create empty FAT tables
    fat = create_empty_fat()

    # Collect files from input directory
    files = []
    for file_path in sorted(input_dir.glob('*')):
        if file_path.is_file():
            files.append(file_path)

    if not files:
        print(f"Warning: No files found in '{input_dir}'")

    print(f"Generating FAT12 image with {len(files)} file(s)...")

    # Create root directory entries
    root_dir = bytearray(ROOT_DIR_SECTORS * SECTOR_SIZE)
    entry_offset = 0

    # Add volume label entry
    volume_entry = create_volume_label_entry("PHANTOM")
    root_dir[entry_offset:entry_offset + 32] = volume_entry
    entry_offset += 32

    # Process files
    current_cluster = 2  # Clusters start at 2 in FAT12
    data_area = bytearray(DATA_SECTORS * SECTOR_SIZE)

    for file_path in files:
        file_size = file_path.stat().st_size

        # Check if file fits
        clusters_needed = (file_size + (SECTORS_PER_CLUSTER * SECTOR_SIZE) - 1) // (SECTORS_PER_CLUSTER * SECTOR_SIZE)
        if current_cluster + clusters_needed > NUM_CLUSTERS + 2:
            print(f"Error: Not enough space for file '{file_path.name}' (need {clusters_needed} clusters)")
            sys.exit(1)

        # Parse filename
        name_parts = file_path.stem[:8], file_path.suffix[1:] if file_path.suffix else ""

        # Create directory entry
        dir_entry = create_directory_entry(
            name_parts[0],
            name_parts[1],
            0x20,  # Archive attribute
            current_cluster,
            file_size
        )
        root_dir[entry_offset:entry_offset + 32] = dir_entry
        entry_offset += 32

        # Read file data
        with open(file_path, 'rb') as f:
            file_data = f.read()

        # Write file data to clusters
        data_offset = (current_cluster - 2) * SECTORS_PER_CLUSTER * SECTOR_SIZE
        data_area[data_offset:data_offset + file_size] = file_data

        # Update FAT chain
        for i in range(clusters_needed):
            cluster = current_cluster + i
            if i == clusters_needed - 1:
                # Last cluster: EOF marker
                fat = write_fat_entry(fat, cluster, 0xFFF)
            else:
                # Point to next cluster
                fat = write_fat_entry(fat, cluster, cluster + 1)

        current_cluster += clusters_needed
        print(f"  Added: {file_path.name} ({file_size} bytes, {clusters_needed} cluster(s))")

    # Write FAT tables to disk image
    fat_offset = FAT1_SECTOR * SECTOR_SIZE
    disk_image[fat_offset:fat_offset + len(fat)] = fat
    fat_offset = FAT2_SECTOR * SECTOR_SIZE
    disk_image[fat_offset:fat_offset + len(fat)] = fat

    # Write root directory to disk image
    root_offset = ROOT_DIR_SECTOR * SECTOR_SIZE
    disk_image[root_offset:root_offset + len(root_dir)] = root_dir

    # Write data area to disk image
    data_offset = DATA_SECTOR * SECTOR_SIZE
    disk_image[data_offset:data_offset + len(data_area)] = data_area

    # Generate C source file
    print(f"Writing C source file to '{output_file}'...")

    with open(output_file, 'w') as f:
        f.write("// Auto-generated FAT12 disk image\n")
        f.write("// Generated by gen_fat12_image.py\n")
        f.write(f"// Source directory: {input_dir}\n")
        f.write(f"// Files embedded: {len(files)}\n\n")

        f.write("#include <stdint.h>\n\n")
        f.write(f"#define MSC_DISK_SECTOR_SIZE {SECTOR_SIZE}\n")
        f.write(f"#define MSC_DISK_SECTOR_COUNT {TOTAL_SECTORS}\n")
        f.write(f"#define MSC_DISK_TOTAL_SIZE ({TOTAL_SECTORS} * {SECTOR_SIZE})\n\n")

        f.write("const uint8_t msc_disk_image[MSC_DISK_TOTAL_SIZE] = {\n")

        # Write bytes in rows of 16
        for i in range(0, len(disk_image), 16):
            chunk = disk_image[i:i+16]
            hex_bytes = ', '.join(f'0x{b:02X}' for b in chunk)
            f.write(f"    {hex_bytes},\n")

        f.write("};\n")

    total_size = len(disk_image)
    used_clusters = current_cluster - 2
    used_bytes = used_clusters * SECTORS_PER_CLUSTER * SECTOR_SIZE

    print(f"\nDisk image generated successfully!")
    print(f"  Total size: {total_size} bytes ({total_size // 1024} KB)")
    print(f"  Used clusters: {used_clusters} / {NUM_CLUSTERS}")
    print(f"  Used space: {used_bytes} bytes ({used_bytes // 1024} KB)")
    print(f"  Free space: {(NUM_CLUSTERS - used_clusters) * SECTOR_SIZE} bytes")
    print(f"  Output: {output_file}")


if __name__ == "__main__":
    main()
