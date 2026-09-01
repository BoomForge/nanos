#!/usr/bin/env python3
"""Audit the V0.30 boot layout, splash hold, FAT12, and M16 fixture."""

from pathlib import Path
import struct
import sys


SECTOR_SIZE = 512
IMAGE_SIZE = 1440 * 1024
EXPECTED_RESERVED = 114
KERNEL_START_SECTOR = 18
KERNEL_RESERVED_SECTORS = 96
FAR_TEST_CLUSTER = 130


def fat12_entry(fat, cluster):
    offset = cluster + cluster // 2
    value = fat[offset] | (fat[offset + 1] << 8)
    return (value >> 4) & 0x0fff if cluster & 1 else value & 0x0fff


def entry_name(entry):
    base = entry[:8].decode("ascii").rstrip()
    extension = entry[8:11].decode("ascii").rstrip()
    return base + ("." + extension if extension else "")


def extract_file(image, fat, entry, data_offset, data_clusters):
    remaining = struct.unpack_from("<I", entry, 28)[0]
    cluster = struct.unpack_from("<H", entry, 26)[0]
    output = bytearray()
    visited = set()
    while remaining:
        assert 2 <= cluster < 2 + data_clusters, (cluster, data_clusters)
        assert cluster not in visited, "FAT12 cluster loop"
        visited.add(cluster)
        offset = data_offset + (cluster - 2) * SECTOR_SIZE
        count = min(remaining, SECTOR_SIZE)
        output.extend(image[offset:offset + count])
        remaining -= count
        if remaining:
            cluster = fat12_entry(fat, cluster)
            assert cluster < 0x0ff8, "FAT12 chain ended early"
    return bytes(output)


def main():
    if len(sys.argv) != 5:
        raise SystemExit(
            "usage: test_milo_storage_contract.py IMAGE FILES_DIR KERNEL STAGE2.S"
        )

    image = Path(sys.argv[1]).read_bytes()
    files_dir = Path(sys.argv[2])
    kernel = Path(sys.argv[3]).read_bytes()
    stage2_source = Path(sys.argv[4]).read_text()
    assert len(image) == IMAGE_SIZE, len(image)
    assert image[3:11] == b"MILO30.0", image[3:11]
    assert b"M.I.L.O stage 1 online" not in image
    assert b"M.I.L.O stage 2 online" not in image
    assert b"M.I.L.O SYSTEM INITIALISING..." in image

    bytes_per_sector = struct.unpack_from("<H", image, 11)[0]
    sectors_per_cluster = image[13]
    reserved = struct.unpack_from("<H", image, 14)[0]
    fat_count = image[16]
    root_entries = struct.unpack_from("<H", image, 17)[0]
    total_sectors = struct.unpack_from("<H", image, 19)[0]
    sectors_per_fat = struct.unpack_from("<H", image, 22)[0]
    assert bytes_per_sector == SECTOR_SIZE
    assert sectors_per_cluster == 1
    assert reserved == EXPECTED_RESERVED
    assert KERNEL_START_SECTOR + KERNEL_RESERVED_SECTORS == reserved
    assert len(kernel) <= KERNEL_RESERVED_SECTORS * SECTOR_SIZE
    kernel_offset = KERNEL_START_SECTOR * SECTOR_SIZE
    assert image[kernel_offset:kernel_offset + len(kernel)] == kernel
    assert b"M.I.L.O VERSION 0.30" in kernel

    for fragment in (
        "call render_status\n    call hold_splash_minimum",
        "hold_splash_minimum:",
        "movl $4, (STAGE2_LOAD_ADDRESS + splash_wait_edges - _start)",
        "movl $0x08000000, %ecx",
        "splash_read_second:",
        "outb %al, $0x70",
        "inb $0x71, %al",
    ):
        assert fragment in stage2_source, fragment

    root_sectors = (root_entries * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE
    fat1_sector = reserved
    fat2_sector = fat1_sector + sectors_per_fat
    root_sector = reserved + fat_count * sectors_per_fat
    data_sector = root_sector + root_sectors
    data_clusters = total_sectors - data_sector
    assert (fat1_sector, fat2_sector, root_sector, data_sector) == (114, 123, 132, 146)
    assert data_clusters == 2734

    fat_size = sectors_per_fat * SECTOR_SIZE
    fat1 = image[fat1_sector * SECTOR_SIZE:fat1_sector * SECTOR_SIZE + fat_size]
    fat2 = image[fat2_sector * SECTOR_SIZE:fat2_sector * SECTOR_SIZE + fat_size]
    assert fat1 == fat2
    root_offset = root_sector * SECTOR_SIZE
    data_offset = data_sector * SECTOR_SIZE
    entries = {}
    for index in range(root_entries):
        entry = image[root_offset + index * 32:root_offset + (index + 1) * 32]
        if entry[0] == 0:
            break
        if entry[0] == 0xe5 or entry[11] == 0x0f:
            continue
        entries[entry_name(entry)] = entry

    source_files = {path.name.upper(): path.read_bytes()
                    for path in files_dir.iterdir() if path.is_file()}
    assert set(entries) == set(source_files), (sorted(entries), sorted(source_files))
    for name, expected in source_files.items():
        actual = extract_file(image, fat1, entries[name], data_offset, data_clusters)
        assert actual == expected, name

    sample = extract_file(image, fat1, entries["MILO.M16"], data_offset,
                          data_clusters)
    assert sample[:4] == b"MI16"
    assert sample[4] == 1
    assert sample[5] == 16
    width, height = struct.unpack_from("<HH", sample, 6)
    assert (width, height) == (96, 64)
    assert len(sample) == 64 + (width * height + 1) // 2

    far_entry = entries["FARTEST.TXT"]
    far_cluster = struct.unpack_from("<H", far_entry, 26)[0]
    assert far_cluster == FAR_TEST_CLUSTER
    assert far_cluster - 2 >= 96
    print("M.I.L.O storage contract: %d clusters, FARTEST.TXT at cluster %d OK" %
          (data_clusters, far_cluster))


if __name__ == "__main__":
    main()
