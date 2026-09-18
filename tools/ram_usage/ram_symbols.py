#!/usr/bin/env python3
"""Find and list the biggest symbols in RAM for the copingTracker firmware.

This is an RP2040 (Raspberry Pi Pico) embedded project. RAM is a scarce
resource (256 KB of SRAM on the RP2040), so it is useful to know which
static/global symbols consume the most RAM.

Approach
--------
1. Run `arm-none-eabi-nm` (binutils) over the built ELF to enumerate every
   defined symbol and its size, in machine readable form:
       <address> <size> <type> <name>
2. Keep only symbols whose address falls inside one of the on-chip RAM
   regions (main RAM 0x20000000-0x2003ffff plus SCRATCH_X/SCRATCH_Y).
   Filtering by address range (not by name) reliably excludes flash
   symbols that live at 0x1000xxxx.
3. Sum the total RAM footprint, then report every symbol whose individual
   size is at least a threshold given either as a percentage of total RAM
   (--min-percent, default 1.0%) or as an absolute byte count (--min-bytes).
   Because the filter is per-symbol (not cumulative), a single very large
   symbol can never hide the rest of the list.

The size reported by `nm` reflects static allocation only (.data + .bss).
It does not capture runtime heap allocations.
"""

import argparse
import os
import shutil
import subprocess
import sys

# RP2040 on-chip SRAM regions (see the linker "Memory Configuration" map).
#   RAM       0x20000000 - 0x2003ffff  (256 KB, .data/.bss/.heap)
#   SCRATCH_X 0x20040000 - 0x20040fff  (4 KB,   core1_stack)
#   SCRATCH_Y 0x20041000 - 0x20041fff  (4 KB)
RAM_START = 0x20000000
RAM_END = 0x20042000  # exclusive


def find_nm():
    """Locate the ARM bare-metal nm, falling back to the host nm."""
    for candidate in ("arm-none-eabi-nm", "nm"):
        path = shutil.which(candidate)
        if path:
            return path
    sys.exit("ERROR: could not find arm-none-eabi-nm or nm on PATH")


def parse_symbols(nm, elf):
    """Run nm and return a list of (address, size, name) for RAM symbols."""
    out = subprocess.check_output(
        [nm, "-S", "--defined-only", "--print-size", elf], text=True
    )
    symbols = []
    for line in out.splitlines():
        parts = line.split(None, 3)
        if len(parts) < 2:
            continue
        try:
            addr = int(parts[0], 16)
        except ValueError:
            continue
        # With -S, sized symbols are "<addr> <size> <type> <name>"; some
        # symbols (e.g. absolute) appear as "<addr> <type> <name>" with no
        # size column. Treat a missing size as zero (skipped below).
        if len(parts) >= 4:
            try:
                size = int(parts[1], 16)
            except ValueError:
                size = 0
            name = parts[3]
        else:
            size = 0
            name = parts[2] if len(parts) >= 3 else ""
        if size == 0:
            continue
        if RAM_START <= addr < RAM_END:
            symbols.append((addr, size, name))
    return symbols


def human(n):
    """Format a byte count in human readable units."""
    if n >= 1024 * 1024:
        return f"{n / (1024 * 1024):.2f} MiB"
    if n >= 1024:
        return f"{n / 1024:.2f} KiB"
    return f"{n} B"


def main():
    parser = argparse.ArgumentParser(
        description="List the biggest static symbols in RAM for copingTracker."
    )
    parser.add_argument(
        "elf",
        nargs="?",
        default="build/Adapters/copingTracker/main/copingTracker.elf",
        help="Path to the ELF binary (default: current build dir)",
    )
    parser.add_argument(
        "--min-percent",
        type=float,
        default=1.0,
        help="Report every symbol whose individual size is at least this "
             "percentage of the total RAM footprint (default: 1.0)",
    )
    parser.add_argument(
        "--min-bytes",
        type=int,
        default=None,
        help="Absolute byte floor: report every symbol of at least this size. "
             "If both --min-percent and --min-bytes are given, the larger "
             "threshold wins.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="List every RAM symbol regardless of size",
    )
    args = parser.parse_args()

    if args.min_percent is not None and not 0 < args.min_percent <= 100:
        sys.exit("ERROR: --min-percent must be in (0, 100]")
    if args.min_bytes is not None and args.min_bytes < 0:
        sys.exit("ERROR: --min-bytes must be >= 0")

    if not os.path.exists(args.elf):
        sys.exit(f"ERROR: ELF not found: {args.elf}")

    nm = find_nm()
    symbols = parse_symbols(nm, args.elf)
    if not symbols:
        sys.exit("ERROR: no RAM-resident symbols found")

    symbols.sort(key=lambda s: s[1], reverse=True)
    total = sum(size for _, size, _ in symbols)

    if args.all:
        reported = symbols
        threshold = 0
    else:
        # Per-symbol threshold (independent of any cumulative summing). Every
        # symbol whose own size is at least the threshold is reported, so a
        # single dominant symbol never hides the rest of the list.
        threshold = total * args.min_percent / 100.0
        if args.min_bytes is not None:
            threshold = max(threshold, args.min_bytes)
        reported = [s for s in symbols if s[1] >= threshold]

    count = len(reported)
    reported_sum = sum(s for _, s, _ in reported)

    # Column widths for a tidy table.
    addr_w = max(len(f"0x{a:08x}") for a, _, _ in reported)
    size_w = max(len(str(s)) for _, s, _ in reported)

    print(f"ELF: {args.elf}")
    print(f"Total static RAM footprint: {human(total)} ({total} bytes)")
    if args.all:
        print("Showing: every RAM symbol (--all)")
    else:
        print(f"Showing: every symbol >= {human(threshold)} "
              f"({args.min_percent:.3g}% of total"
              + (f", or min {args.min_bytes} B" if args.min_bytes is not None else "")
              + ")")
    print(f"Symbols shown: {count} of {len(symbols)} "
          f"(summing {human(reported_sum)})")
    print()

    header = f"{'Address':<{addr_w}}  {'Size (B)':>{size_w}}  {'Size':>8}  Name"
    print(header)
    print("-" * len(header))
    for addr, size, name in reported:
        print(f"0x{addr:08x}  {size:>{size_w}}  {human(size):>8}  {name}")

    print()
    print("Note: sizes reflect static allocation (.data + .bss); runtime heap is not included.")


if __name__ == "__main__":
    main()
