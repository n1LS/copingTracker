import os
import shutil

import convert_font

# The three font variants, in the order the UI indexes them
# (index 0 = light/regular, 1 = bold, 2 = graphic).
FONTS = [
    ("light", "font_light.png"),
    ("bold", "font_bold.png"),
    ("graphics", "font_graphics.png"),
]

GLYPHS_PER_FONT = 256
GLYPH_H = convert_font.GLYPH_H


def format_uint16_rows(rows):
    return ", ".join(f"0x{value:03X}" for value in rows)


def build_combined_data():
    """Process each font image and merge the data into single contiguous arrays."""
    combined_glyphs = []
    combined_masks = []
    combined_mask_indices = []
    mask_offsets = []

    for name, png in FONTS:
        glyphs, masks, mask_indices = convert_font.process_image(png, 128, 0, 255)

        if len(glyphs) != GLYPHS_PER_FONT:
            raise RuntimeError(f"{name}: expected {GLYPHS_PER_FONT} glyphs, got {len(glyphs)}")

        mask_offsets.append(len(combined_masks))
        combined_glyphs.extend(glyphs)
        combined_masks.extend(masks)

        # Rebase this font's mask indices so they point into the single
        # contiguous mask array (they were relative to this font's own masks).
        combined_mask_indices.extend(v + mask_offsets[-1] if v != -1 else -1 for v in mask_indices)

    return combined_glyphs, combined_masks, combined_mask_indices, mask_offsets


def write_header(out, glyphs, masks, mask_indices):
    font_count = len(FONTS)
    total_glyphs = font_count * GLYPHS_PER_FONT

    out.write("/* This file is part of the copingTracker firmware. It is auto-generated and should no be edited. */\n\n")
    out.write("#ifndef FONT_H\n")
    out.write("#define FONT_H\n\n")
    out.write("#include <stdint.h>\n\n")

    out.write(f"#define FONT_COUNT {font_count}\n")
    out.write(f"#define FONT_GLYPHS {GLYPHS_PER_FONT}\n")
    out.write(f"#define FONT_MASK_TOTAL {len(masks)}\n\n")

    # Single contiguous bitmap: FONT_COUNT * FONT_GLYPHS rows of GLYPH_H each.
    out.write(f"static const uint16_t font_bitmaps[{total_glyphs}][{GLYPH_H}] = {{\n")
    for index, glyph in enumerate(glyphs):
        font_idx = index // GLYPHS_PER_FONT
        code = index % GLYPHS_PER_FONT
        row_hex = format_uint16_rows(glyph)

        if 32 <= code <= 126:
            character = chr(code)
            if code == ord("\\"):
                character = "backslash"
            elif code == ord('"'):
                character = "double quote"
            comment = f" // {FONTS[font_idx][0]} {character}"
        else:
            comment = f" // {FONTS[font_idx][0]} {code}"

        out.write(f"    {{{row_hex}}},{comment}\n")
    out.write("};\n\n")

    # Single contiguous mask-index array; values are absolute positions in
    # font_masks_all (-1 when a glyph has no mask).
    out.write(f"static const int8_t font_mask_indices[{total_glyphs}] = {{\n")
    for offset in range(0, total_glyphs, 8):
        values = mask_indices[offset:offset + 8]
        formatted_values = ", ".join(str(value) for value in values)
        out.write(f"    {formatted_values},\n")
    out.write("};\n\n")

    # Single contiguous mask array holding the masks of all fonts. A one-entry
    # zero-filled array is emitted when no glyph contains fuchsia, because
    # zero-length arrays are not standard C.
    mask_array_size = max(1, len(masks))
    out.write(f"static const uint16_t font_masks_all[{mask_array_size}][{GLYPH_H}] = {{\n")
    if masks:
        for index, mask in enumerate(masks):
            formatted_rows = format_uint16_rows(mask)
            out.write(f"    {{{formatted_rows}}}, // {index}\n")
    else:
        zero_rows = ", ".join("0x000" for _ in range(GLYPH_H))
        out.write(f"    {{{zero_rows}}}, // unused; no masked glyphs\n")
    out.write("};\n\n")

    out.write("#endif // FONT_H\n")


def main():
    glyphs, masks, mask_indices, _mask_offsets = build_combined_data()

    with open("font.h", "w") as out:
        write_header(out, glyphs, masks, mask_indices)

    # Copy the same generated header into both firmware targets.
    shutil.copyfile("font.h", "../../sources/Adapters/Host/display/font.generated.h")
    os.rename("font.h", "../../sources/Adapters/copingTracker/display/font.generated.h")


if __name__ == "__main__":
    main()

