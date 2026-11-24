#!/usr/bin/env python3
"""
SPDX-License-Identifier: BSD-3-Clause

Copyright (c) 2026 nILS Podewski

This file is part of the copingTracker firmware

Convert ContextLegends.copingDoc into ContextLegend.generated.h, the compiled
data that drives the context legend (the live button/combo help that views
show in the info area at the bottom of the screen, see View::drawContextLegend).

Every legend is a block of 5 three-line groups (fg / bg / text), like the
command help in Commands.copingDoc:

  group 1      the label of the legend. The text row holds the name of the
               ContextLegend enum entry that is generated for the block
               (e.g. "SongSelection"). Its fg/bg rows are not used.
  groups 2-5   the four rows of the legend (fg colours / bg colours / text)

Every line is 30 bytes wide. The legend is drawn from column 5 of the screen, so
the first 27 columns fit on screen; the last 3 columns are ignored.

Usage:
  python3 convert-contextlegends.py [ContextLegends.copingDoc] [output.generated.h]
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SOURCE = ROOT / "tools/manual/raw_data/ContextLegends.copingDoc"
DEFAULT_OUTPUT = ROOT / "sources/Application/Utils/ContextLegend.generated.h"

TEXTROWS = 4  # legend rows per block
GROUPS = 1 + TEXTROWS  # label group + legend rows
LINEWIDTH = 30
LEGEND_WIDTH = 27  # the legend starts at column 5 of the 32 column screen
RESERVED = {"None"}

LABEL_RE = re.compile(r"^[A-Za-z][A-Za-z0-9]*$")

HEADER_BANNER = """/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 *
 * GENERATED FILE - DO NOT EDIT
 *
 * This data was generated from
 * tools/manual/raw_data/ContextLegends.copingDoc
 *
 * To regenerate:
 *   python3 tools/manual/raw_data/convert-contextlegends.py
 * or simply run ./build.py (generation is part of the prebuild steps).
 *
 * Every line is trimmed from 30 columns to the 27 that fit the legend area
 * on screen.
 *
 * Include this header wherever context legends are needed.
 */
"""


def c_string(data):
    """Bytes -> C string literal.

    Hex escapes in C are greedy ("\\x9FAb" is ONE escape), so a literal is
    closed and reopened whenever a hex digit follows an escape.
    """
    out = ['"']
    after_escape = False
    for b in data:
        if b == 0x22:  # "
            out.append('\\"')
            after_escape = False
        elif b == 0x5C:  # backslash
            out.append("\\\\")
            after_escape = False
        elif 0x20 <= b < 0x7F:
            ch = chr(b)
            if after_escape and ch in "0123456789abcdefABCDEF":
                out.append('" "')
            out.append(ch)
            after_escape = False
        else:
            out.append(f"\\x{b:02X}")
            after_escape = True
    out.append('"')
    return "".join(out)


def load_lines(source):
    content = source.read_bytes()
    lines = [ln.rstrip(b"\r") for ln in content.split(b"\n")]
    while lines and lines[-1] == b"":
        lines.pop()
    if not lines or len(lines) % (GROUPS * 3) != 0:
        raise RuntimeError(
            f"{source}: expected a multiple of {GROUPS * 3} lines "
            f"({GROUPS} groups x 3 lines per legend), got {len(lines)}"
        )
    for idx, ln in enumerate(lines, 1):
        if len(ln) != LINEWIDTH:
            raise RuntimeError(
                f"{source}:{idx}: line is {len(ln)} bytes, expected {LINEWIDTH}"
            )
    return lines


def make_tint_block(fg, bg, txt, is_last, indent=8):
    pad = " " * indent
    tail = "" if is_last else ","
    return [
        pad + "makeTintString(",
        pad + "  " + c_string(fg[:LEGEND_WIDTH]) + ",",
        pad + "  " + c_string(bg[:LEGEND_WIDTH]) + ",",
        pad + "  " + c_string(txt[:LEGEND_WIDTH]),
        pad + ")" + tail,
    ]


def read_label(source, block, text_row):
    raw = text_row.rstrip(b" ").decode("ascii", errors="replace")
    if not LABEL_RE.match(raw):
        raise RuntimeError(
            f"{source}, legend {block + 1}: the label '{raw}' (text row of the "
            f"first group) is not a valid enum name"
        )
    if raw in RESERVED:
        raise RuntimeError(
            f"{source}, legend {block + 1}: '{raw}' is reserved"
        )
    return raw


def generate(source, output):
    lines = load_lines(source)
    count = len(lines) // (GROUPS * 3)

    labels = []
    blocks = []
    for block in range(count):
        base = block * GROUPS * 3
        groups = []
        for g in range(GROUPS):
            off = base + g * 3
            groups.append((lines[off], lines[off + 1], lines[off + 2]))

        label = read_label(source, block, groups[0][2])
        if label in labels:
            raise RuntimeError(
                f"{source}, legend {block + 1}: duplicate label '{label}'"
            )
        labels.append(label)
        blocks.append(groups[1:])
        print(f"  - added {label}")

    out = [HEADER_BANNER, "", "// clang-format off", ""]

    out.append("#ifndef _CONTEXT_LEGEND_GENERATED_H_")
    out.append("#define _CONTEXT_LEGEND_GENERATED_H_")
    out.append("")
    out.append('#include "Application/Utils/TintChar.h"')
    out.append("#include <stdint.h>")
    out.append("")
    out.append("// One legend as it is shown in the info area at the bottom of a view")
    out.append("struct ContextLegendText {")
    for n in range(1, TEXTROWS + 1):
        out.append(f"  etl::array<TintChar, {LEGEND_WIDTH + 1}> line{n};")
    out.append("};")
    out.append("")
    out.append("enum class ContextLegend : uint8_t {")
    out.append("  None,")
    for label in labels:
        out.append(f"  {label},")
    out.append("};")
    out.append("")

    out.append("inline ContextLegendText getContextLegend(ContextLegend legend) {")
    out.append("  switch (legend) {")
    for label, rows in zip(labels, blocks):
        out.append(f"    case ContextLegend::{label}:")
        out.append("      return ContextLegendText(")
        for r, (fg, bg, txt) in enumerate(rows):
            out.extend(make_tint_block(fg, bg, txt, is_last=(r == TEXTROWS - 1)))
        out.append("      );")
        out.append("")
    out.append("    case ContextLegend::None:")
    out.append("      break;")
    out.append("  }")
    out.append("")
    out.append("  // nothing to show")
    out.append("  return ContextLegendText(")
    blank = '"' + " " * LEGEND_WIDTH + '"'
    for r in range(TEXTROWS):
        tail = "" if r == TEXTROWS - 1 else ","
        out.append("        makeTintString(")
        out.append(f"          {blank},")
        out.append(f"          {blank},")
        out.append(f"          {blank}")
        out.append("        )" + tail)
    out.append("  );")
    out.append("}")
    out.append("")
    out.append("#endif /* _CONTEXT_LEGEND_GENERATED_H_ */")
    out.append("")
    out.append("// clang-format on")

    Path(output).write_text("\n".join(out) + "\n")
    print(f"Wrote {output} ({count} legends)")


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SOURCE
    output = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUTPUT
    try:
        generate(source, output)
    except RuntimeError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
