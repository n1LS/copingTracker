#!/usr/bin/env python3
from PIL import Image

img = Image.open("font_light.png")
written = 0

with open("font.F10", "wb") as f:

    for char in range(256):
        char_x = (char % 16) * 10
        char_y = (char // 16) * 10

        for y in range(10):
            value = 0
            for x in range(10):
                r, g, b = img.getpixel((char_x + x, char_y + y))[:3]
                if r < 10 and g < 10 and b < 10:
                    value |= 1 << x

            f.write(value.to_bytes(2, "little"))
            written += 1

print(f'Written {written} bytes.')