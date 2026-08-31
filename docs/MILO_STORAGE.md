# M.I.L.O V0.21 full-volume FAT12 storage

V0.21 keeps the fast boot cache while separating its size from the capacity of
the FAT12 volume. The kernel can now load, edit, search, copy, delete, and write
files whose clusters lie anywhere on the 1.44 MB boot floppy.

## Disk layout

| Sectors | Purpose |
| --- | --- |
| 0 | Stage 1 and FAT12 BIOS parameter block |
| 1-16 | Stage 2 loader |
| 17 | Reserved spacer |
| 18-81 | 32 KiB kernel reservation |
| 82-90 | FAT copy 1 |
| 91-99 | FAT copy 2 |
| 100-113 | 224-entry root directory |
| 114-2879 | 2,766 single-sector data clusters |

The kernel reservation is a ceiling, not the executable size. V0.21 remains
well below that ceiling and the build rejects an oversized kernel.

## Read path

Stage 2 still loads both FAT copies, the complete root directory, and the first
96 data clusters into the 64 KiB cache at `0x20000`. This preserves fast access
for common small files. A cluster outside that window is translated to its
absolute FAT12 sector and fetched through the loader's BIOS read bridge into a
dedicated sector buffer at `0x32000`.

The editor buffer remains an independent 8 KiB workspace at `0x30000`. A file
may live anywhere on disk while still respecting the deliberate 8,191-byte text
editing limit.

## Write path

Allocation and free-space reporting scan all 2,766 data clusters. Near writes
update their cached sector directly; far writes use the dedicated sector
buffer. Every data, FAT, and root-directory sector write is read back into a
separate verification buffer at `0x32200` and compared before success is
reported.

The filesystem remains intentionally small: FAT12, root-directory files, 8.3
names, 512-byte clusters, and no subdirectories. These constraints are enough
for the offline document system while keeping the implementation auditable.

## Regression fixture

`FARTEST.TXT` is deliberately placed at cluster 130. That is beyond the 96
cached data clusters. If `type FARTEST.TXT` displays its contents, the runtime
has completed an on-demand disk read rather than accidentally using the cache.
The host storage contract independently verifies the boot layout, both FAT
copies, every packaged file, the far placement, and the full-volume capacity.
