#!/usr/bin/env python3

'''
'' SPDX-License-Identifier: BSD-3-Clause
''
'' Copyright (c) 2026 nILS Podewski
''
'' This file is part of the copingTracker firmware
'''

import argparse
import datetime
import os
import sys
import textwrap

from groq import Groq


WIDTH = 30

SYSTEM_PROMPT = """\
You edit release notes for a small music tracker.
Each line of input text is a single entry.
For each entry:
- Correct spelling and grammar and improve wording where useful.
- Attempt to reformat into past tense action items as like "fixed xyz..." even if the line is a bug report like "item does not show up" 

Rules:
- Keep the original meaning.
- The output must have the same number of lines as the input
- Keep technical terms unchanged.
- Do not invent features or details.
- Keep it concise or if possible make it more concise.
- try to keep each line under 90 characters.
- Return only the final release-note text, one item per line.
- No puncuation at the end of eah item.
- Remove additional infos that might be given in parantheses.
- Only use characters defined in ASCII (32-127).
"""


def convert(lines, client):
    """Send one release-note item to Groq for editing."""

    response = client.chat.completions.create(
        model="openai/gpt-oss-120b",
        messages=[
            {
                "role": "system",
                "content": SYSTEM_PROMPT,
            },
            {
                "role": "user",
                "content": "\n".join(lines),
            },
        ],
        temperature=0.2,
    )
    return response.choices[0].message.content.strip().split("\n")


def wrap_item(text):
    """Wrap a list item so every line is exactly WIDTH characters."""
    prefix = "- "
    continuation_prefix = "  "
    content_width = WIDTH - len(prefix)

    words = text.split()
    lines = []
    current = ""

    for word in words:
        if len(word) > content_width:
            if current:
                lines.append(current)
                current = ""

            while len(word) > content_width:
                lines.append(word[:content_width])
                word = word[content_width:]

            if word:
                current = word
            continue

        if not current:
            current = word
        elif len(current) + 1 + len(word) <= content_width:
            current += " " + word
        else:
            lines.append(current)
            current = word

    if current:
        lines.append(current)

    if not lines:
        lines = [""]

    result = []

    for index, line in enumerate(lines):
        prefix = "- " if index == 0 else "  "
        result.append((prefix + line).ljust(WIDTH))

    return result


def coping_doc_lines(text, fg="F", bg="0"):
    """Convert one line into copingDoc's three-line format."""
    assert len(text) == WIDTH
    return [
        fg.ljust(WIDTH),
        bg.ljust(WIDTH),
        text,
    ]


def main():
    print("Changelog Assembler ------------------------------------------------------------")
    
    parser = argparse.ArgumentParser(
        description="Convert Done items into copingDoc release notes."
    )
    parser.add_argument("source")
    parser.add_argument("copingdoc")
    parser.add_argument("versionheader")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="do not modify either file",
    )

    args = parser.parse_args()

    versionResult = version_string_exists(args.versionheader, args.copingdoc)

    if versionResult is True:
        print("[DONE] Aborting, current version is already in the log file.")
        exit(0)


    # Read and validate everything before making any changes.
    with open(args.source, "r", encoding="utf-8") as file:
        source_lines = file.readlines()

    done_index = None
    todo_index = None

    for index, line in enumerate(source_lines):
        if line.strip() == "Done:":
            done_index = index

        if line.strip() == "ToDo:":
            todo_index = index

    if done_index is None:
        print("[WARN] source file does not contain 'Done:'.", file=sys.stderr)
        return 1

    if todo_index is None:
        print("[WARN] source file does not contain 'ToDo:'.",  file=sys.stderr)
        return 1

    if todo_index <= done_index:
        print("[WARN] 'ToDo:' must appear after 'Done:'.", file=sys.stderr)
        return 1

    done_lines = [line.strip() for line in source_lines[done_index + 1:todo_index] if line.strip()]

    if len(done_lines) == 0:
        print("[Done] No changes listed in ToDo file")
        exit(0)

    # Set up Groq.
    api_key = os.environ.get("GROQ_API_KEY")

    if not api_key:
        print(
            "Error: GROQ_API_KEY environment variable is not set.",
            file=sys.stderr,
        )
        return 1

    client = Groq(api_key=api_key)

    converted_lines = convert(done_lines, client)

    print("CONVERSION RESULTS:")

    for n in range(len(converted_lines)):
        print(f"{n+1}. {done_lines[n]}\n\t -> {converted_lines[n]}")

    # Build the copingDoc content.
    output_lines = []

    for text in converted_lines:
        for wrapped_line in wrap_item(text):
            output_lines.extend(
                coping_doc_lines(
                    wrapped_line,
                    fg="F",
                    bg="0",
                )
            )

    output_text = ""
    if output_lines:
        output_text = "\n".join(output_lines) + "\n"

    # Read existing copingDoc before making any changes.
    try:
        with open(args.copingdoc, "r", encoding="utf-8") as file:
            existing_output = file.read()
    except FileNotFoundError:
        existing_output = ""

    updated_source = (
        source_lines[:done_index + 1]
        + source_lines[todo_index:]
    )

    updated_output = output_text + existing_output

    # check if we need to prepend the version information

    print(f"VERSION CHECK RESULT: {versionResult}")

    if versionResult is not True:
        today = datetime.date.today().isoformat()
        header = (versionResult + " " + today).ljust(30)
        output_lines.insert(0, header)
        output_lines.insert(0, "9                               ")
        output_lines.insert(0, "F                               ")
    else:
        print("Current version is already in the log, aborting.")

    if args.dry_run:
        print("\nDry run - no files modified.")
        print(f"Would remove {len(done_lines)} line(s) from {args.source}.")
        print(
            f"Would add {len(output_lines) // 3} "
            f"copingDoc line(s) to {args.copingdoc}:"
        )
        print("\n".join(output_lines))
        return 0

    # Commit the changes.
    with open(args.source, "w", encoding="utf-8") as file:
        file.writelines(updated_source)

    with open(args.copingdoc, "w", encoding="utf-8") as file:
        file.write(updated_output)

    print(
        f"\nProcessed {len(converted_lines)} release note(s)."
    )

    return 0


import re


def version_string_exists(header_file, log_file):
    with open(header_file, "r") as f:
        header = f.read()

    values = {}

    for name, value in re.findall(
        r'#define\s+(PRODUCT_\w+)\s+"([^"]*)"', header
    ):
        values[name] = value

    version_string = (
        "v"
        + values["PRODUCT_VERSION"]
        + values["PRODUCT_RELEASE"]
    )

    with open(log_file, "r") as f:
        if version_string in f.read():
            return True
        else:
            return "v" + values["PRODUCT_VERSION"] + values["PRODUCT_RELEASE"]

if __name__ == "__main__":
    sys.exit(main())    