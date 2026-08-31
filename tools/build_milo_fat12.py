#!/usr/bin/env python3
"""Populate the M.I.L.O boot image with a small, valid FAT12 filesystem."""

import os
import struct
import sys


SECTOR_SIZE = 512


def fat_name(filename):
    base, extension = os.path.basename(filename).upper().split(".", 1)
    if not base or len(base) > 8 or len(extension) > 3:
        raise ValueError("FAT12 requires an 8.3 filename: %s" % filename)
    return (base.ljust(8) + extension.ljust(3)).encode("ascii")


def set_fat12_entry(fat, cluster, value):
    offset = cluster + cluster // 2
    if cluster & 1:
        current = fat[offset] | (fat[offset + 1] << 8)
        current = (current & 0x000f) | ((value & 0x0fff) << 4)
    else:
        current = fat[offset] | (fat[offset + 1] << 8)
        current = (current & 0xf000) | (value & 0x0fff)
    fat[offset] = current & 0xff
    fat[offset + 1] = (current >> 8) & 0xff


def main():
    if len(sys.argv) < 3:
        raise SystemExit(
            "usage: build_milo_fat12.py IMAGE FILES_DIR [--place NAME=CLUSTER]")

    placements = {}
    for argument in sys.argv[3:]:
        if not argument.startswith("--place="):
            raise ValueError("unknown option: %s" % argument)
        value = argument.split("=", 1)[1]
        name, cluster_text = value.rsplit("=", 1)
        placements[name.upper()] = int(cluster_text)

    with open(sys.argv[1], "rb") as image:
        boot_sector = image.read(SECTOR_SIZE)
    bytes_per_sector = struct.unpack_from("<H", boot_sector, 11)[0]
    sectors_per_cluster = boot_sector[13]
    reserved_sectors = struct.unpack_from("<H", boot_sector, 14)[0]
    fat_count = boot_sector[16]
    root_entries = struct.unpack_from("<H", boot_sector, 17)[0]
    total_sectors = struct.unpack_from("<H", boot_sector, 19)[0]
    sectors_per_fat = struct.unpack_from("<H", boot_sector, 22)[0]
    if bytes_per_sector != SECTOR_SIZE or sectors_per_cluster != 1:
        raise ValueError("M.I.L.O requires 512-byte, single-sector clusters")
    root_sectors = (root_entries * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE
    fat_offset = reserved_sectors * SECTOR_SIZE
    root_offset = (reserved_sectors + fat_count * sectors_per_fat) * SECTOR_SIZE
    data_offset = root_offset + root_sectors * SECTOR_SIZE
    data_clusters = total_sectors - data_offset // SECTOR_SIZE

    filenames = sorted(
        os.path.join(sys.argv[2], name)
        for name in os.listdir(sys.argv[2])
        if os.path.isfile(os.path.join(sys.argv[2], name))
    )
    fat = bytearray(sectors_per_fat * SECTOR_SIZE)
    fat[0:3] = b"\xf0\xff\xff"
    root = bytearray(root_sectors * SECTOR_SIZE)
    data_blocks = {}
    next_cluster = 2

    for entry_index, filename in enumerate(filenames):
        if entry_index >= root_entries:
            raise ValueError("too many FAT12 root entries")
        with open(filename, "rb") as source:
            contents = source.read()
        cluster_count = max(1, (len(contents) + SECTOR_SIZE - 1) // SECTOR_SIZE)
        requested_cluster = placements.get(os.path.basename(filename).upper())
        if requested_cluster is not None:
            if requested_cluster < next_cluster:
                raise ValueError("requested cluster overlaps earlier files: %s" % filename)
            next_cluster = requested_cluster
        if next_cluster - 2 + cluster_count > data_clusters:
            raise ValueError("files do not fit on the FAT12 volume")
        first_cluster = next_cluster
        for cluster_index in range(cluster_count):
            cluster = next_cluster
            next_cluster += 1
            following = 0x0fff if cluster_index + 1 == cluster_count else next_cluster
            set_fat12_entry(fat, cluster, following)
            block = contents[cluster_index * SECTOR_SIZE:(cluster_index + 1) * SECTOR_SIZE]
            data_blocks[cluster] = block.ljust(SECTOR_SIZE, b"\0")

        offset = entry_index * 32
        root[offset:offset + 11] = fat_name(filename)
        root[offset + 11] = 0x20
        struct.pack_into("<H", root, offset + 26, first_cluster)
        struct.pack_into("<I", root, offset + 28, len(contents))

    with open(sys.argv[1], "r+b") as image:
        image.seek(fat_offset)
        image.write(fat)
        image.write(fat)
        image.seek(root_offset)
        image.write(root)
        for cluster, block in sorted(data_blocks.items()):
            image.seek(data_offset + (cluster - 2) * SECTOR_SIZE)
            image.write(block)

    print("M.I.L.O FAT12: %d files, %d data clusters" % (
        len(filenames), len(data_blocks)))


if __name__ == "__main__":
    main()
