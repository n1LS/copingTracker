#!/usr/bin/env python3
"""
SPDX-License-Identifier: BSD-3-Clause

Copyright (c) 2026 nILS Podewski

This file is part of the copingTracker firmware

Convert Commands.copingDoc (the on-device "Commands" manual page) into
CommandHelp.generated.h, the compiled data that drives the on-screen
command legend (Application/View / drawCommandLegend -> getCommandHelp).

Commands.copingDoc is the single source of truth for both the manual page
and the command legend. Every command occupies 5 three-line groups
(fg / bg / text): 3 teaching rows + 1 continuation row + 1 box-drawing
separator row. Every line is 30 bytes wide. The last 4 bytes of each line
are manual-page layout metadata: a 0xB3 line-break marker at column 26
followed by a 3-letter command abbreviation at columns 27..29 (e.g. 'Arp').
The legend only has room for 26 columns, so those 4 bytes are trimmed off;
the abbreviation is used to map each block to its Token.

Usage:
  python3 convert-commandhelp.py [Commands.copingDoc] [output.generated.h]
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SOURCE = ROOT / "tools/manual/raw_data/Commands.copingDoc"
DEFAULT_OUTPUT = ROOT / "sources/Application/Utils/CommandHelp.generated.h"
TYPES_HEADER = ROOT / "sources/Foundation/Types/Types.h"

TEXTROWS = 4  # text rows per command (before the separator row)
GROUPS = TEXTROWS + 1  # 4 text rows + 1 separator
LINEWIDTH = 30
LEGEND_WIDTH = 26
ABBR_START = 27  # 3-letter command abbreviation at columns 27..29
BREAKER = 0xB3  # manual-page line-break marker at column 26

TOKEN_RE = re.compile(r'ETL_ENUM_TYPE_16\((\w+),\s*"([^"]*)"\)')

HEADER_BANNER = """/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 *
 * GENERATED FILE - DO NOT EDIT
 *
 * This data was generated from the on-device Commands manual page,
 * tools/manual/raw_data/Commands.copingDoc, which is the single source of
 * truth for both the manual page and the command legend.
 *
 * To regenerate:
 *   python3 tools/manual/raw_data/convert-commandhelp.py
 * or simply run ./build.py (generation is part of the prebuild steps).
 *
 * Every manual line is trimmed from 30 columns to the 26 that fit the
 * command legend on screen. The trailing 3-letter command abbreviation
 * and the 0xB3 line-break marker are manual-page layout metadata and are
 * not part of the visible text.
 *
 * This header is included from Application/Utils/CommandHelp.h and is
 * never meant to be included directly.
 */
"""


def load_tokens():
    """abbreviation (uppercased) -> full InstrumentCommand Token name."""
    tokens = {}
    for name, abbr in TOKEN_RE.findall(TYPES_HEADER.read_text()):
        if not name.startswith("InstrumentCommand"):
            continue
        if name == "InstrumentCommandNone":
            continue
        key = abbr.upper()
        if key in tokens:
            raise RuntimeError(
                f"{TYPES_HEADER.name}: duplicate abbreviation '{abbr}' "
                f"({name} vs {tokens[key]})"
            )
        tokens[key] = name
    return tokens


def c_string(data):
    """Bytes -> C string literal, escaping quotes/backslashes/non-printables."""
    out = ['"']
    for b in data:
        if b == 0x22:  # "
            out.append('\\"')
        elif b == 0x5C:  # backslash
            out.append("\\\\")
        elif 0x20 <= b < 0x7F:
            out.append(chr(b))
        else:
            out.append(f"\\x{b:02X}")
    out.append('"')
    return "".join(out)


def load_lines(source):
    content = source.read_bytes()
    lines = [ln.rstrip(b"\r") for ln in content.split(b"\n")]
    while lines and lines[-1] == b"":
        lines.pop()
    expected = 30 * GROUPS * 3
    if len(lines) != expected:
        raise RuntimeError(
            f"{source}: expected {expected} lines "
            f"(30 commands x {GROUPS} rows x 3 channels), got {len(lines)}"
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


def generate(source, output):
    tokens = load_tokens()
    lines = load_lines(source)

    out = [HEADER_BANNER, "", "// clang-format off", "",
           "CommandHelp getCommandHelp(Token command) {",
           "  switch (command) {"]

    covered = set()
    breaker_mismatch = 0
    for block in range(30):
        base = block * GROUPS * 3
        groups = []
        for g in range(GROUPS):
            off = base + g * 3
            groups.append((lines[off], lines[off + 1], lines[off + 2]))
        separator = groups[TEXTROWS]
        if separator[2][0] != 0xC4:
            raise RuntimeError(
                f"{source}, command {block + 1}: the row after the fourth text row "
                f"must be a box-drawing separator (starts with 0xC4); "
                f"is the source file intact?"
            )
        text_rows = groups[:TEXTROWS]

        raw_abbr = text_rows[0][2][ABBR_START:ABBR_START + 3]
        abbr = raw_abbr.decode("ascii", errors="replace")
        token = tokens.get(abbr.upper())
        if token is None:
            raise RuntimeError(
                f"{source}, command {block + 1}: abbreviation {raw_abbr!r} "
                f"(columns 28-30) does not match any instrument command in Types.h"
            )
        if token in covered:
            raise RuntimeError(
                f"{source}, command {block + 1}: duplicate command '{token}'"
            )
        covered.add(token)

        for fg, bg, txt in text_rows:
            if len(txt) >= ABBR_START and txt[26] != BREAKER:
                breaker_mismatch += 1

        out.append(f"    case Token::{token}:")
        out.append("      return CommandHelp(")
        for r, (fg, bg, txt) in enumerate(text_rows):
            out.extend(make_tint_block(fg, bg, txt, is_last=(r == TEXTROWS - 1)))
        out.append("      );")
        out.append("")

    leftover = set(tokens.values()) - covered
    if leftover:
        raise RuntimeError(
            f"{source} is missing command help for: {sorted(leftover)}. "
            f"Every instrument command needs a block on the Commands manual page."
        )

    out.append("      default:")
    out.append("        break;")
    out.append("    }")
    out.append("")
    out.append("    // This should never happen")
    out.append("    return CommandHelp(")
    blank_rows = [
        (b"9" + b" " * 29, b"0" + b" " * 29, b"This should never happen. "),
        (b"0" + b" " * 29, b"0" + b" " * 29, b" " * 29),
        (b"0" + b" " * 29, b"0" + b" " * 29, b" " * 29),
        (b"0" + b" " * 29, b"0" + b" " * 29, b" " * 29),
    ]
    for r, (fg, bg, txt) in enumerate(blank_rows):
        out.extend(make_tint_block(fg, bg, txt, is_last=(r == 3), indent=6))
    out.append("    );")
    out.append("  }")
    out.append("  // clang-format on")
    out.append("")

    output.write_text("\n".join(out) + "\n")
    print(f"Wrote {output} ({len(covered)} commands)")
    if breaker_mismatch:
        print(f"  note: {breaker_mismatch} text row(s) missing a 0xB3 break marker at column 26")


def main():
    source = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else DEFAULT_SOURCE
    output = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else DEFAULT_OUTPUT
    if not source.is_file():
        raise SystemExit(f"source not found: {source}")
    generate(source, output)


if __name__ == "__main__":
    main()

