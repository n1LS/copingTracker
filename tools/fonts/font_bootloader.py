from PIL import Image

CELL_SIZE = 8

GRID_W = 16
GRID_H = 14

FIRST_CHAR = 32
LAST_CHAR = 255

EMPTY_CHAR = 63 # "?" character

img = Image.open("font_bootloader.png").convert("RGBA")

pixels = img.load()

if pixels == None:
    raise Exception("Failed to load font image")

font = []
char_map = [EMPTY_CHAR] * (LAST_CHAR - FIRST_CHAR)

glyphIndex = 0

for charCode in range(FIRST_CHAR, LAST_CHAR):
    cell = charCode - FIRST_CHAR

    gx = cell % GRID_W
    gy = cell // GRID_W

    baseX = gx * CELL_SIZE
    baseY = gy * CELL_SIZE

    rows = []
    empty = True

    for y in range(8):
        value = 0

        for x in range(8):
            r, g, b, a = pixels[baseX + x, baseY + y]

            bit = 0

            if r == g == b == 0:
                bit = 1

            if bit:
                empty = False
                value |= (1 << x)

        rows.append(value)

    # keep first glyph even if empty
    if charCode == FIRST_CHAR or not empty:
        font.append(rows)
        char_map[charCode - FIRST_CHAR] = glyphIndex
        glyphIndex += 1

print('''/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include <stdint.h>
''')

print("uint8_t font[][8] = {")

for glyph in font:
    print("    {", end="")

    for i, row in enumerate(glyph):
        if i != 0:
            print(", ", end="")

        print(f"0x{row:02X}", end="")

    print("},")

print("};")
print()

print(f"uint8_t char_map[{LAST_CHAR - FIRST_CHAR}] =")
print("{")

for i, value in enumerate(char_map):
    if i % 16 == 0:
        print("    ", end="")

    print(f"0x{value:02X}, ", end="")

    if (i + 1) % 16 == 0:
        print()

print("\n};\n")