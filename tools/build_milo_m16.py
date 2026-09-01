#!/usr/bin/env python3
"""Build the tiny indexed M16 V1 image shipped with M.I.L.O V0.29."""

from pathlib import Path
import struct
import sys


WIDTH = 96
HEIGHT = 64
PALETTE = (
    (7, 4, 11),       # desktop black
    (17, 11, 22),     # panel
    (32, 21, 37),     # dark edge
    (67, 33, 77),     # active purple
    (131, 107, 136),  # muted lilac
    (214, 166, 242),  # lilac
    (239, 112, 232),  # M.I.L.O pink
    (111, 228, 170),  # status green
    (242, 232, 244),  # text white
    (239, 102, 132),  # warning rose
    (92, 49, 105),
    (180, 82, 174),
    (50, 32, 57),
    (119, 82, 132),
    (228, 208, 235),
    (0, 0, 0),
)

GLYPHS = {
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
}


def plot(pixels, x, y, colour):
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        pixels[y][x] = colour


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: build_milo_m16.py OUTPUT.M16")

    pixels = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

    # Sparse Vesper grid.
    for y in range(0, HEIGHT, 8):
        for x in range(WIDTH):
            if (x + y) % 3:
                plot(pixels, x, y, 1)
    for x in range(0, WIDTH, 12):
        for y in range(HEIGHT):
            if (x + y) % 4:
                plot(pixels, x, y, 1)

    # Distressed M.I.L.O ring.
    cx, cy = WIDTH // 2, HEIGHT // 2
    for y in range(HEIGHT):
        for x in range(WIDTH):
            distance2 = (x - cx) ** 2 + (y - cy) ** 2
            if 25 ** 2 <= distance2 <= 27 ** 2 and (x + y) % 7:
                pixels[y][x] = 6
            elif 22 ** 2 <= distance2 <= 23 ** 2 and (x * 3 + y) % 11:
                pixels[y][x] = 5

    # Four compact block letters.
    scale = 2
    word_width = (5 * 4 + 3) * scale
    cursor_x = (WIDTH - word_width) // 2
    top = 25
    for character in "MILO":
        glyph = GLYPHS[character]
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit == "1":
                    for sy in range(scale):
                        for sx in range(scale):
                            plot(pixels, cursor_x + gx * scale + sx,
                                 top + gy * scale + sy, 8)
        cursor_x += 6 * scale

    # Tiny heart/ring state below the word.
    for x, y in ((46, 44), (49, 44), (45, 45), (46, 45), (47, 45),
                 (48, 45), (49, 45), (50, 45), (46, 46), (47, 46),
                 (48, 46), (49, 46), (47, 47), (48, 47)):
        plot(pixels, x, y, 6)
    plot(pixels, 48, 47, 7)

    header = bytearray(64)
    header[0:4] = b"MI16"
    header[4] = 1
    header[5] = len(PALETTE)
    struct.pack_into("<HH", header, 6, WIDTH, HEIGHT)
    for index, (red, green, blue) in enumerate(PALETTE):
        header[16 + index * 3:19 + index * 3] = bytes((red, green, blue))

    packed = bytearray()
    flat = [pixel for row in pixels for pixel in row]
    for index in range(0, len(flat), 2):
        high = flat[index] & 0x0F
        low = flat[index + 1] & 0x0F if index + 1 < len(flat) else 0
        packed.append((high << 4) | low)

    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(header + packed)
    print("M.I.L.O M16: %dx%d, %d colours, %d bytes" %
          (WIDTH, HEIGHT, len(PALETTE), len(header) + len(packed)))


if __name__ == "__main__":
    main()
