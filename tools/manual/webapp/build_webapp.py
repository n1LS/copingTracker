#!/usr/bin/env python3

'''
'' SPDX-License-Identifier: BSD-3-Clause
''
'' Copyright (c) 2026 nILS Podewski
''
'' This file is part of the copingTracker firmware
'''

"""
Builds a standalone HTML manual from the same sources the firmware uses:

    tools/manual/raw_data/Documentation.rc   page list and titles
    tools/manual/raw_data/*.copingDoc        page content
    tools/fonts/font_light.png               glyph atlas

Everything (glyphs, text, palette) is inlined into a single index.html so the
result can be opened straight from disk.
"""

import argparse
import base64
import json
import sys
from pathlib import Path

from PIL import Image

# Must match HelpView.cpp / convert-documentation.py
DOC_COLUMNS = 30

# Must match tools/fonts/convert_font.py
GLYPH_COLS = 16
GLYPH_ROWS = 16
GLYPH_W = 10
GLYPH_H = 10
GLYPH_COUNT = GLYPH_COLS * GLYPH_ROWS

# Default theme, mirroring ThemeConstants.h
PALETTE = [
    0x000000, 0xBB3B2A, 0x25BC24, 0xC88200,
    0x003259, 0xBF4182, 0x2DB1BE, 0x808080,
    0x303030, 0xFC391F, 0x31E71F, 0xFFC023,
    0x40649E, 0xFF64B4, 0x14F0F0, 0xDEDEDE,
]


def hex_value(c, filename, line_no):
    """A blank means 'keep the previous colour', mirroring the firmware tool."""
    if c == " ":
        return None
    if c in "0123456789abcdefABCDEF":
        return int(c, 16)
    raise RuntimeError(f"{filename}:{line_no}: invalid hex digit '{c}'")


def parse_copingdoc(path):
    """Parse one .copingDoc into rows of [colourByte, charCode] pairs.

    Same 3-line (fg / bg / text) block structure and same sticky-colour rules
    as convert-documentation.py, so the webapp and the firmware agree.
    """
    rows = []
    last_fg = 0
    last_bg = 0
    line_no = 0

    with open(path, "rb") as f:
        while True:
            fg = f.readline()
            if not fg:
                break

            bg = f.readline()
            txt = f.readline()
            line_no += 3

            if not bg or not txt:
                raise RuntimeError(f"{path}:{line_no}: incomplete 3-line block")

            fg = fg.rstrip(b"\r\n")
            bg = bg.rstrip(b"\r\n")
            txt = txt.rstrip(b"\r\n")

            if len(fg) != len(bg) or len(fg) != len(txt):
                raise RuntimeError(
                    f"{path}:{line_no}: line length mismatch "
                    f"({len(fg)}, {len(bg)}, {len(txt)})"
                )

            cells = []
            for i in range(len(txt)):
                new_fg = hex_value(chr(fg[i]), path, line_no)
                new_bg = hex_value(chr(bg[i]), path, line_no)

                if new_fg is not None:
                    last_fg = new_fg
                if new_bg is not None:
                    last_bg = new_bg

                cells.append(((last_fg << 4) | last_bg, txt[i]))

            if len(cells) != DOC_COLUMNS:
                raise RuntimeError(
                    f"{path}:{line_no}: expected {DOC_COLUMNS} columns, got {len(cells)}"
                )

            rows.append(cells)

    return rows


def parse_rc(rc_path):
    documents = []
    with open(rc_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(";", 1)
            if len(parts) != 2:
                raise RuntimeError(f"{rc_path}: invalid entry '{line}'")
            documents.append({"filename": parts[0].strip(), "title": parts[1].strip()})
    return documents


def build_glyph_atlas(font_path):
    """Pack the font into a 1-bit-per-pixel atlas.

    The PNG is RGBA with transparent background; a pixel is "on" when it is
    opaque and dark, matching convert_font.py's threshold. Fuchsia mask pixels
    only matter for the in-app highlight effect and none of the glyphs used by
    the documentation carry them, so they are treated as background here.

    Returns a base64 string of GLYPH_COUNT * GLYPH_H bytes, where each byte
    holds one glyph row (bit 9..0 = leftmost..rightmost pixel).
    """
    image = Image.open(font_path).convert("RGBA")
    width, height = image.size

    expected = (GLYPH_COLS * GLYPH_W, GLYPH_ROWS * GLYPH_H)
    if (width, height) != expected:
        raise SystemExit(f"{font_path}: expected {expected[0]}x{expected[1]}, got {width}x{height}")

    px = image.load()

    # Two bytes per row so a 10 pixel wide glyph fits.
    data = bytearray()

    for code in range(GLYPH_COUNT):
        left = (code % GLYPH_COLS) * GLYPH_W
        top = (code // GLYPH_COLS) * GLYPH_H

        for y in range(GLYPH_H):
            bits = 0
            for x in range(GLYPH_W):
                r, g, b, a = px[left + x, top + y]
                if a != 0 and (r + g + b) < 48:
                    bits |= 1 << (GLYPH_W - 1 - x)
            data.append(bits & 0xFF)
            data.append((bits >> 8) & 0xFF)

    return base64.b64encode(bytes(data)).decode("ascii")


def build_payload(rc_path, font_path):
    base_dir = Path(rc_path).resolve().parent
    documents = parse_rc(rc_path)

    pages = []
    for doc in documents:
        path = base_dir / doc["filename"]
        if not path.exists():
            raise RuntimeError(f"Missing file: {path}")

        rows = parse_copingdoc(path)

        # Flatten to two parallel compact strings: colours as hex pairs and
        # text as raw code points. Keeps the inlined payload small and avoids
        # a huge nested JSON array.
        colors = "".join(f"{c:02X}" for row in rows for c, _ in row)
        chars = [ch for row in rows for _, ch in row]

        pages.append(
            {
                "title": doc["title"],
                "rows": len(rows),
                "colors": colors,
                "chars": chars,
            }
        )

    return {
        "columns": DOC_COLUMNS,
        "glyphWidth": GLYPH_W,
        "glyphHeight": GLYPH_H,
        "palette": [f"#{c:06X}" for c in PALETTE],
        "atlas": build_glyph_atlas(font_path),
        "pages": pages,
    }


def main():
    here = Path(__file__).resolve().parent
    repo = here.parents[2]

    parser = argparse.ArgumentParser(description="Build the standalone HTML manual")
    parser.add_argument(
        "--rc",
        default=str(repo / "tools/manual/raw_data/Documentation.rc"),
        help="Documentation.rc listing the pages",
    )
    parser.add_argument(
        "--font",
        default=str(repo / "tools/fonts/font_light.png"),
        help="glyph atlas PNG",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=str(here / "index.html"),
        help="output HTML file",
    )
    args = parser.parse_args()

    payload = build_payload(Path(args.rc), Path(args.font))

    template = (here / "template.html").read_text(encoding="utf-8")
    script = (here / "app.js").read_text(encoding="utf-8")

    # Inline everything so the page works straight from the filesystem, where
    # fetch() of a sibling file would be blocked.
    data = json.dumps(payload, separators=(",", ":"))

    # The payload sits inside a <script> block, so it must not contain a
    # sequence that would close it early.
    data = data.replace("</", "<\\/")

    html = template.replace("/*__DATA__*/", data).replace("/*__SCRIPT__*/", script)

    out = Path(args.output)
    out.write_text(html, encoding="utf-8")

    total_rows = sum(p["rows"] for p in payload["pages"])
    print(
        f"Generated {out} "
        f"({len(payload['pages'])} pages, {total_rows} rows, {len(html) / 1024:.0f} KB)"
    )


if __name__ == "__main__":
    main()
