#!/usr/bin/env python3

'''
'' SPDX-License-Identifier: BSD-3-Clause
''
'' Copyright (c) 2026 nILS Podewski
''
'' This file is part of the copingTracker firmware
'''

import sys
from pathlib import Path


HEADER = """/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <stdint.h>

"""


def enum_name(name):
    """
    Convert filename into enum identifier:
    General -> docIdGeneral
    my_page -> docIdMyPage
    """
    parts = (
        name.replace(".copingDoc", "")
        .replace("-", "_")
        .replace(".", "_")
        .split("_")
    )

    return "docId" + "".join(p[:1].upper() + p[1:] for p in parts if p)


def array_name(name):
    """
    Convert filename into array identifier:
    General -> DOC_GENERAL
    """
    return (
        "DOC_"
        + name.replace(".copingDoc", "")
        .replace("-", "_")
        .replace(".", "_")
        .upper()
    )


def hex_value(c, filename, line_no):
    if c == " ":
        return None

    if c in "0123456789abcdefABCDEF":
        return int(c, 16)

    raise RuntimeError(
        f"{filename}:{line_no}: invalid hex digit '{c}'"
    )


def write_array(out, name, filename):
    array = array_name(name)

    print(f"static const uint8_t {array}[] =", file=out)
    print("{", file=out)

    last_fg = 0
    last_bg = 0
    line_no = 0

    with open(filename, "rb") as f:
        while True:
            fg = f.readline()
            if not fg:
                break

            bg = f.readline()
            txt = f.readline()

            line_no += 3

            if not bg or not txt:
                raise RuntimeError(
                    f"{filename}:{line_no}: incomplete 3-line block"
                )

            fg = fg.rstrip(b"\r\n")
            bg = bg.rstrip(b"\r\n")
            txt = txt.rstrip(b"\r\n")

            if len(fg) != len(bg) or len(fg) != len(txt):
                raise RuntimeError(
                    f"{filename}:{line_no}: line length mismatch "
                    f"({len(fg)}, {len(bg)}, {len(txt)})"
                )

            values = []

            for i in range(len(txt)):
                # Convert ASCII byte to character for hex_value()
                new_fg = hex_value(chr(fg[i]), filename, line_no)
                new_bg = hex_value(chr(bg[i]), filename, line_no)

                if new_fg is not None:
                    last_fg = new_fg

                if new_bg is not None:
                    last_bg = new_bg

                color = (last_fg << 4) | last_bg

                values.append(f"0x{color:02X}")
                values.append(f"0x{txt[i]:02X}")

            print("    " + ",".join(values) + ",", file=out)

    print("};\n", file=out)


def main():
    if len(sys.argv) != 3:
        print(
            f"Usage: {sys.argv[0]} <documentation.rc> <Documentation.generated.h>"
        )
        sys.exit(1)

    rc_file = Path(sys.argv[1]).resolve()
    output_file = Path(sys.argv[2])

    base_dir = rc_file.parent

    documents = []

    with open(rc_file, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            parts = line.split(";", 1)

            if len(parts) != 2:
                raise RuntimeError(
                    f"{rc_file}: invalid entry '{line}' "
                    "(expected filename;Title)"
                )

            filename, title = parts

            documents.append(
                {
                    "filename": filename.strip(),
                    "title": title.strip(),
                }
            )

    with open(output_file, "w", encoding="utf-8") as out:
        out.write(HEADER)

        # Enum
        print("enum DocumentationId", file=out)
        print("{", file=out)

        for doc in documents:
            print(
                f"    {enum_name(doc['filename'])},",
                file=out,
            )

        print("    docIdCount", file=out)
        print("};\n", file=out)

        # Data arrays
        for doc in documents:
            path = base_dir / doc["filename"]

            if not path.exists():
                raise RuntimeError(f"Missing file: {path}")

            write_array(
                out,
                doc["filename"],
                path,
            )

        # Lookup structure
        print("struct DocumentationPage", file=out)
        print("{", file=out)
        print("    const char *title;", file=out)
        print("    const uint8_t *data;", file=out)
        print("    uint32_t size;", file=out)
        print("};\n", file=out)

        print(
            "static const DocumentationPage documentation[docIdCount] =",
            file=out,
        )
        print("{", file=out)

        for doc in documents:
            arr = array_name(doc["filename"])

            print(
                f'    {{ "{doc["title"]}", {arr}, sizeof({arr}) }},',
                file=out,
            )

        print("};", file=out)

    print(f"Generated {output_file}")


if __name__ == "__main__":
    main()