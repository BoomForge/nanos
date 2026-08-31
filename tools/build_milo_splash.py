#!/usr/bin/env python3
"""Convert the M.I.L.O ANSI portrait into the compact boot-splash format."""

import re
import struct
import sys
from collections import Counter


WIDTH = 100
HEIGHT = 30
PALETTE_SIZE = 16
ANSI_CELL = re.compile(r"\x1b\[38;5;(\d+)m(.)", re.DOTALL)


def xterm_rgb(index):
    base = (
        (0, 0, 0), (128, 0, 0), (0, 128, 0), (128, 128, 0),
        (0, 0, 128), (128, 0, 128), (0, 128, 128), (192, 192, 192),
        (128, 128, 128), (255, 0, 0), (0, 255, 0), (255, 255, 0),
        (0, 0, 255), (255, 0, 255), (0, 255, 255), (255, 255, 255),
    )
    if index < 16:
        return base[index]
    if index < 232:
        value = index - 16
        blue = value % 6
        value //= 6
        green = value % 6
        red = value // 6
        levels = (0, 95, 135, 175, 215, 255)
        return levels[red], levels[green], levels[blue]
    gray = 8 + (index - 232) * 10
    return gray, gray, gray


def color_distance(left, right):
    return sum((a - b) * (a - b) for a, b in zip(left, right))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: build_milo_splash.py INPUT.ans OUTPUT.bin")

    with open(sys.argv[1], "rb") as source:
        text = source.read().decode("latin1")

    parsed = ANSI_CELL.findall(text)
    cells = [(int(color), char) for color, char in parsed if char not in "\r\n"]
    if len(cells) != WIDTH * HEIGHT:
        raise SystemExit("expected %d ANSI cells, found %d" % (WIDTH * HEIGHT, len(cells)))

    visible_counts = Counter(color for color, char in cells if char != " ")
    selected = [color for color, _ in visible_counts.most_common(PALETTE_SIZE - 1)]
    palette_rgb = [(0, 0, 0)] + [xterm_rgb(color) for color in selected]

    records = []
    for position, (color, char) in enumerate(cells):
        if char == " ":
            continue
        rgb = xterm_rgb(color)
        palette_index = min(
            range(1, len(palette_rgb)),
            key=lambda index: color_distance(rgb, palette_rgb[index]),
        )
        records.append((position, ord(char), palette_index))

    while len(palette_rgb) < PALETTE_SIZE:
        palette_rgb.append((0, 0, 0))

    with open(sys.argv[2], "wb") as output:
        output.write(b"MSP1")
        output.write(struct.pack("<HHHH", WIDTH, HEIGHT, len(records), PALETTE_SIZE))
        for red, green, blue in palette_rgb:
            output.write(struct.pack("<I", (red << 16) | (green << 8) | blue))
        for position, char, palette_index in records:
            output.write(struct.pack("<HBB", position, char, palette_index))

    print("M.I.L.O splash: %dx%d, %d visible cells, %d bytes" % (
        WIDTH, HEIGHT, len(records), 12 + PALETTE_SIZE * 4 + len(records) * 4))


if __name__ == "__main__":
    main()
