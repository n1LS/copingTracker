#!/usr/bin/env python3
"""
png2font_c.py

Usage:
    python3 png2font_c.py font.png [--threshold 128]

Reads a 160x160 PNG containing a 16x16 grid of 10x10 glyphs.
Glyph index 0 corresponds to the character specified by --start.

The generated header contains:

    static const uint16_t FONT_NAME_BITMAP[COUNT][10];
    static const int8_t   FONT_NAME_MASK_INDEX[COUNT];
    static const uint16_t FONT_NAME_MASK[MASK_COUNT][10];

Pixel handling:

    - Transparent pixels: background in the font bitmap
    - Exact fuchsia pixels (255, 0, 255): background in the font bitmap
      and set in the mask bitmap
    - All other pixels: set according to the grayscale threshold

The existing orientation is preserved:
    - source X coordinate becomes the output row
    - source Y coordinate becomes the output bit position
"""

import argparse
import os
import re

from PIL import Image


GLYPH_COLS = 16
GLYPH_ROWS = 16
GLYPH_W = 10
GLYPH_H = 10

FUCHSIA = (255, 0, 255)
FUCHSIA_TOLERANCE = round(255 * 0.35)

def is_fuchsia_pixel(red, green, blue):
    return all(
        abs(actual - expected) <= FUCHSIA_TOLERANCE
        for actual, expected in zip(
            (red, green, blue),
            FUCHSIA,
        )
    )

def sanitize_ident(name: str) -> str:
    """Convert a name to an uppercase C identifier."""
    identifier = re.sub(r"[^0-9a-zA-Z_]", "_", name)

    if not identifier:
        identifier = "FONT"

    if re.match(r"^\d", identifier):
        identifier = "_" + identifier

    return identifier.upper()


def convert_glyph(rgba, left, top, threshold):
    """
    Convert one glyph into:

        font_rows
        mask_rows
        contains_fuchsia

    The source-to-output orientation matches the original implementation.
    """
    font_rows = []
    mask_rows = []
    contains_fuchsia = False

    for out_row in range(GLYPH_W):
        font_row_value = 0
        mask_row_value = 0

        for out_col in range(GLYPH_H):
            src_x = left + out_row
            src_y = top + (GLYPH_H - 1 - out_col)

            red, green, blue, alpha = rgba[src_x, src_y]

            is_fuchsia = is_fuchsia_pixel(red, green, blue)

            bit_index = GLYPH_H - 1 - out_col
            bit = 1 << bit_index

            # Fuchsia is always represented in the mask, regardless of alpha.
            if is_fuchsia:
                mask_row_value |= bit
                contains_fuchsia = True

            # Fuchsia and transparent pixels are never set in the font bitmap.
            elif (red + green + blue) < 48:
                font_row_value |= bit

        font_rows.append(font_row_value)
        mask_rows.append(mask_row_value)

    return font_rows, mask_rows, contains_fuchsia


def process_image(img_path, threshold, start, end):
    """
    Process the image and return:

        glyphs
        masks
        mask_indices

    mask_indices uses the selected character range as its index range:

        mask_indices[0] corresponds to character `start`
        mask_indices[code - start] corresponds to character `code`
    """
    glyph_count = end - start + 1

    image = Image.open(img_path)
    rgba_image = image.convert("RGBA")
    grayscale_image = image.convert("L")

    width, height = rgba_image.size

    expected_width = GLYPH_COLS * GLYPH_W
    expected_height = GLYPH_ROWS * GLYPH_H

    if (width, height) != (expected_width, expected_height):
        raise SystemExit(
            f"ERROR: image size must be "
            f"{expected_width}x{expected_height}. Got {width}x{height}"
        )

    rgba_pixels = rgba_image.load()

    all_glyphs = []
    all_masks = []
    all_masked = []

    for glyph_y in range(GLYPH_ROWS):
        for glyph_x in range(GLYPH_COLS):
            left = glyph_x * GLYPH_W
            top = glyph_y * GLYPH_H

            font_rows, mask_rows, contains_fuchsia = convert_glyph(
                rgba_pixels,
                left,
                top,
                threshold,
            )

            all_glyphs.append(font_rows)
            all_masks.append(mask_rows)
            all_masked.append(contains_fuchsia)

    selected_glyphs = all_glyphs[start:end + 1]
    selected_masks = all_masks[start:end + 1]
    selected_masked = all_masked[start:end + 1]

    if len(selected_glyphs) != glyph_count:
        raise RuntimeError("Internal error: unexpected glyph selection count")

    glyphs = []
    masks = []
    mask_indices = [-1] * glyph_count

    for glyph_slot, (glyph, mask, is_masked) in enumerate(
        zip(selected_glyphs, selected_masks, selected_masked)
    ):
        glyphs.append(glyph)

        if is_masked:
            mask_indices[glyph_slot] = len(masks)
            masks.append(mask)

    return glyphs, masks, mask_indices


def format_uint16_rows(rows):
    return ", ".join(f"0x{value:03X}" for value in rows)


def write_bitmap_array(file_handle, name, rows_list, array_size=None):
    """
    Write a uint16_t bitmap array.

    If rows_list is empty, write one unused zero-filled entry. This avoids
    generating a non-standard zero-length C array.
    """
    actual_size = array_size if array_size is not None else len(rows_list)

    file_handle.write(
        f"static const uint16_t {name}[{actual_size}][{GLYPH_H}] = {{\n"
    )

    if rows_list:
        for index, rows in enumerate(rows_list):
            formatted_rows = format_uint16_rows(rows)
            file_handle.write(f"    {{{formatted_rows}}}, // {index}\n")
    else:
        zero_rows = ", ".join("0x000" for _ in range(GLYPH_H))
        file_handle.write(
            f"    {{{zero_rows}}}, // unused; no masked glyphs\n"
        )

    file_handle.write("};\n\n")


def write_mask_index_array(file_handle, name, mask_indices, start):
    """Write the mask-index array with character-code comments."""
    file_handle.write(
        f"static const int8_t {name}[{len(mask_indices)}] = {{\n"
    )

    for offset in range(0, len(mask_indices), 8):
        values = mask_indices[offset:offset + 8]
        first_code = start + offset
        last_code = first_code + len(values) - 1

        formatted_values = ", ".join(str(value) for value in values)

        if first_code == last_code:
            comment = f"character {first_code}"
        else:
            comment = f"characters {first_code}..{last_code}"

        file_handle.write(
            f"    {formatted_values}, // {comment}\n"
        )

    file_handle.write("};\n\n")


def write_header(font_name, glyphs, masks, mask_indices, out_path, start):
    """Write the generated C header."""
    identifier = sanitize_ident(font_name)

    bitmap_name = f"FONT_{identifier}_BITMAP"
    mask_index_name = f"FONT_{identifier}_MASK_INDEX"
    mask_name = f"FONT_{identifier}_MASK"

    with open(out_path, "w", newline="\n") as file_handle:
        file_handle.write("#include <stdint.h>\n\n")

        file_handle.write(
            f"// Character range: {start}..{start + len(glyphs) - 1}\n"
        )
        file_handle.write(
            f"// Glyph count: {len(glyphs)}\n"
        )
        file_handle.write(
            f"// Masked glyph count: {len(masks)}\n\n"
        )

        file_handle.write(
            f"static const uint16_t {bitmap_name}"
            f"[{len(glyphs)}][{GLYPH_H}] = {{\n"
        )

        code = start

        for glyph in glyphs:
            row_hex = format_uint16_rows(glyph)

            if 32 <= code <= 126:
                character = chr(code)

                if code == ord("\\"):
                    character = "backslash"
                elif code == ord('"'):
                    character = "double quote"

                comment = f" // {character}"
            else:
                comment = f" // {code}"

            file_handle.write(
                f"    {{{row_hex}}},{comment}\n"
            )

            code += 1

        file_handle.write("};\n\n")

        write_mask_index_array(
            file_handle,
            mask_index_name,
            mask_indices,
            start,
        )

        # A one-entry zero-filled array is emitted when no glyph contains
        # fuchsia, because zero-length arrays are not standard C.
        mask_array_size = max(1, len(masks))

        write_bitmap_array(
            file_handle,
            mask_name,
            masks,
            array_size=mask_array_size,
        )


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Convert a 16x16 PNG grid of 10x10 glyphs "
            "into a C header bitmap and optional masks"
        )
    )

    parser.add_argument(
        "png",
        help="input PNG filename; expected size is 160x160",
    )
    parser.add_argument(
        "--threshold",
        "-t",
        type=int,
        default=128,
        help="grayscale threshold for non-transparent, non-fuchsia pixels",
    )
    parser.add_argument(
        "--out",
        "-o",
        help="output header filename",
    )
    parser.add_argument(
        "--start",
        "-s",
        type=int,
        default=32,
        help="first character code represented by the first glyph",
    )
    parser.add_argument(
        "--end",
        "-e",
        type=int,
        default=127,
        help="last character code represented by the last glyph",
    )
    parser.add_argument(
        "--name",
        "-n",
        type=str,
        default="wide",
        help="font name used in the generated C identifiers",
    )

    args = parser.parse_args()

    if not os.path.exists(args.png):
        raise SystemExit(f"Input file not found: {args.png}")

    if not 0 <= args.threshold <= 255:
        raise SystemExit("ERROR: threshold must be between 0 and 255")

    if not 0 <= args.start <= 255:
        raise SystemExit("ERROR: start must be between 0 and 255")

    if not 0 <= args.end <= 255:
        raise SystemExit("ERROR: end must be between 0 and 255")

    if args.start > args.end:
        raise SystemExit("ERROR: start must not be greater than end")

    if args.end - args.start + 1 > GLYPH_COLS * GLYPH_ROWS:
        raise SystemExit(
            f"ERROR: character range cannot contain more than "
            f"{GLYPH_COLS * GLYPH_ROWS} characters"
        )

    output_path = args.out
    if output_path is None:
        output_path = f"font_{args.name.lower()}.h"

    glyphs, masks, mask_indices = process_image(
        args.png,
        args.threshold,
        args.start,
        args.end,
    )

    write_header(
        args.name,
        glyphs,
        masks,
        mask_indices,
        output_path,
        args.start,
    )

    print(
        f"Wrote {output_path} "
        f"({len(glyphs)} glyphs, {len(masks)} masked glyphs, "
        f"{GLYPH_W}x{GLYPH_H} each)"
    )


if __name__ == "__main__":
    main()