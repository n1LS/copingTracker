#!/usr/bin/env python3
"""Report how the copingTracker firmware's RAM is split across *kinds* of symbols.

Where ram_symbols.py lists the biggest *individual* static symbols in RAM, this
script answers a complementary question:

    "Where does RAM go in bulk?"

Many symbols are individually small but belong to the same family (e.g. all the
static buffers inside picoTrackerSystem::Boot(), or every etl::singleton<T>
instance, or the dozens of file-local globals). Individually each is invisible
at a 1% threshold; summed together they can be the biggest consumers of the
RP2040's 256 KB of SRAM.

Approach
--------
1. Enumerate every defined symbol with its size via `arm-none-eabi-nm`
   (same machine readable form as ram_symbols.py, and the same RAM address
   range filter: 0x20000000-0x2003ffff plus SCRATCH_X/SCRATCH_Y).
2. Demangle each symbol name through `c++filt` / `arm-none-eabi-c++filt`.
3. Classify every symbol into a broad "kind" bucket based on the structure of
   its demangled name:
       * singletons      - etl::singleton<T> and T_Factory<T> instances (+ guards)
       * static members  - a class's static data members (Type::member)
       * function-locals - statics local to a function (Func()::local)
       * file-locals     - static file-scope globals, anonymous-namespace statics
       * runtime/library - newlib/libc, tinyusb, SDIO/UART driver internals
       * other           - anything the heuristics above do not recognise
   Each bucket is further split into per-class/per-function subgroups so the
   rollup is actionable (see the indented sub-totals under each bucket).
4. Sum sizes per (bucket, subgroup), sort by total bytes descending, and print
   the biggest chunks first.

The classifier is a heuristic tuned for this codebase and lives in one function
(``classify``) so it is easy to extend.

The size reported by `nm` reflects static allocation only (.data + .bss) plus
code that is copied into RAM. It does not capture runtime heap allocation.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

# RP2040 on-chip SRAM regions (identical to ram_symbols.py).
#   RAM       0x20000000 - 0x2003ffff  (256 KB, .data/.bss/.heap)
#   SCRATCH_X 0x20040000 - 0x20040fff  (4 KB,   core1_stack)
#   SCRATCH_Y 0x20041000 - 0x20041fff  (4 KB)
RAM_START = 0x20000000
RAM_END = 0x20042000  # exclusive


def find_tool(*candidates):
    """Return the first tool on PATH among candidates, else exit."""
    for candidate in candidates:
        path = shutil.which(candidate)
        if path:
            return path
    sys.exit(
        "ERROR: could not find any of: " + ", ".join(candidates) + " on PATH"
    )


def parse_symbols(nm, elf):
    """Run nm and return a list of (address, size, name) for RAM symbols.

    Mirrors ram_symbols.py: keeps every defined symbol whose address falls in an
    on-chip RAM region, dropping symbols with no reported size (most notably
    absolute symbols, which nm prints without a size column).
    """
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
        # Sized symbols are "<addr> <size> <type> <name>"; some symbols
        # (e.g. absolute) appear as "<addr> <type> <name>" with no size.
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


def demangle_all(cxxfilt, names):
    """Demangle a list of symbol names in one batch via c++filt."""
    if not names:
        return {}
    proc = subprocess.run(
        [cxxfilt], input="\n".join(names), capture_output=True, text=True
    )
    out = proc.stdout.splitlines()
    return {name: out[i] for i, name in enumerate(names) if i < len(out)}


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------

# Substrings in the *raw* (undemangled) name that mark a symbol as belonging to
# the low-level runtime / third-party library layer rather than application
# code. These appear in .data/.bss and in the vector table / code copied to RAM.
_RUNTIME_RAW_MARKERS = (
    "_Unwind",
    "__lock_",
    "__malloc",
    "_atexit",
    "atexit",
    "_stdio",
    "errno",
    "__sf",
    # tinyusb / USB stack internals
    "_tusb",
    "_usbd_",
    "_mscd_",
    "_msc_",
    "_cdcd_",
    "_midid_",
    "_ctrl_",
    "_sof_enable",
    "hw_endpoints",
    "dcd_rp2040_irq",
    "ram_vector_table",
    "ram_hash",
    # SDIO / UART driver globals
    "g_sdio",
    "sd_table",
    "sf_table",
    "sf_clz_func",
    "static_linear_",
    "m_stream_",
    "uart_inst",
    "uart_instance",
    "midi_rx_queue",
    "msd_sd_",
    "rosc_samples",
    "rng_",
    "_desc_str",
    "boot2_copyout",
)

# Newlib/libc-style prefixes in the *demangled* form.
_RUNTIME_DEMANGLED_MARKERS = (
    "_Unwind",
    "__",
    "errno",
    "lazy_vsnprintf",
    "boot2_copyout",
)


def _strip_guard(name):
    """Return the guarded target if ``name`` is a function-static guard.

    Guards such as "guard variable for etl::singleton<Config>::get_data()::data"
    are folded into the kind of the thing they guard. Returns None when ``name``
    is not a guard.
    """
    prefix = "guard variable for "
    if name.startswith(prefix):
        return name[len(prefix):]


def classify(raw_name, demangled):
    """Map a symbol onto a (bucket, subgroup) pair.

    ``bucket`` is one of the coarse kinds; ``subgroup`` is a more specific key
    used for sub-totals within the bucket (owning class, function, type, ...).
    Returns a (bucket, subgroup) tuple; subgroup is never empty.
    """
    name = demangled or raw_name

    # Guard variables: keep their size with the thing they guard so a static
    # object plus its thread-safe-init guard are counted together.
    guarded = _strip_guard(name)
    if guarded is not None:
        return classify(raw_name, guarded)

    # etl::singleton<T> instances.
    m = re.search(r"etl::singleton<([^>]+)>::get_data\(\)::data", name)
    if m:
        return ("singletons", "etl::singleton<{}>".format(m.group(1).strip()))
    # T_Factory<T> instances.
    m = re.search(r"T_Factory<([^>]+)>::instance_", name)
    if m:
        return ("singletons", "T_Factory<{}>".format(m.group(1).strip()))
    # Generic `<Singleton>::instance_` / `<Singleton>::instance` singletons.
    m = re.match(r"([A-Za-z_][\w:<>]*)\b::(?:instance_|instance)\b", name)
    if m:
        return ("singletons", m.group(1))

    # Function-local statics:  Func()::local  or  Class::Func()::local.
    m = re.search(r"((?:[\w:]+)?::)?([\w~]+)\([^)]*\)::([\w.]+)", name)
    if m:
        func = m.group(2)
        owner = (m.group(1) or "").strip(":")
        subgroup = "{}{}()".format((owner + "::") if owner else "", func)
        return ("function-locals", subgroup)

    # Static data members:  Type::member  (no parentheses in the tail).
    m = re.match(r"^([\w:<>, ]+)::([\w.]+)$", name)
    if m and not m.group(1).startswith("("):
        return ("static members", m.group(1).strip())

    # Guard leftovers and anonymous-namespace/file-local names.
    if name.startswith("guard variable for"):
        return ("file-locals", "(guards)")

    # Low level runtime / library symbols.
    if any(mk in raw_name for mk in _RUNTIME_RAW_MARKERS) or any(
        mk in name for mk in _RUNTIME_DEMANGLED_MARKERS
    ):
        return _runtime_bucket(name)

    # Everything else is a file-scope static (usually _ZL...).
    return ("file-locals", name or "(unnamed)")


def _runtime_bucket(name):
    """Pick a subgroup name for a runtime/library symbol."""
    low = name.lower()
    if "_unwind" in low or "_gnu_" in low or "terminate" in low:
        sub = "newlib/unwind"
    elif any(k in low for k in ("_usbd_", "_mscd_", "_cdcd_", "_midid_",
                                "_ctrl_", "_tusb", "hw_endpoints",
                                "dcd_rp2040_irq", "ram_vector_table",
                                "ram_hash", "_sof_enable", "_desc_str")):
        sub = "tinyusb/usb"
    elif any(k in low for k in ("g_sdio", "sd_table", "sf_table", "sf_clz_func",
                                "static_linear_", "m_stream_", "uart_inst",
                                "msd_sd_", "midi_rx_queue", "rosc_samples",
                                "rng_", "boot2_copyout", "claimed", "io_")):
        sub = "sdio/uart/driver"
    elif any(k in low for k in ("__lock_", "__malloc", "__sf", "atexit",
                                "__stdio", "errno", "lazy_vsnprintf")):
        sub = "newlib/libc"
    else:
        sub = "runtime (other)"
    return ("runtime/library", sub)



# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def human(n):
    """Format a byte count in human readable units."""
    if n >= 1024 * 1024:
        return "{:.2f} MiB".format(n / (1024 * 1024))
    if n >= 1024:
        return "{:.2f} KiB".format(n / 1024)
    return "{} B".format(n)


def main():
    parser = argparse.ArgumentParser(
        description="Group the static RAM usage of copingTracker by symbol kind."
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
        default=0.5,
        help="Hide any group whose total size is below this percentage of the "
             "RAM footprint (default: 0.5).",
    )
    parser.add_argument(
        "--min-bytes",
        type=int,
        default=None,
        help="Absolute byte floor: hide groups smaller than this. If both "
             "--min-percent and --min-bytes are given, the larger wins.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Show every group regardless of size",
    )
    args = parser.parse_args()

    if args.min_percent is not None and not 0 < args.min_percent <= 100:
        sys.exit("ERROR: --min-percent must be in (0, 100]")
    if args.min_bytes is not None and args.min_bytes < 0:
        sys.exit("ERROR: --min-bytes must be >= 0")

    if not os.path.exists(args.elf):
        sys.exit("ERROR: ELF not found: {}".format(args.elf))

    nm = find_tool("arm-none-eabi-nm", "nm")
    cxxfilt = find_tool("arm-none-eabi-c++filt", "c++filt")

    symbols = parse_symbols(nm, args.elf)
    if not symbols:
        sys.exit("ERROR: no RAM-resident symbols found")

    total = sum(size for _, size, _ in symbols)

    # Demangle the unique names once.
    names = {name for _, _, name in symbols}
    demangled = demangle_all(cxxfilt, list(names))

    # Classify and aggregate: {(bucket, subgroup): [sum, count, largest]}
    groups = {}
    for addr, size, raw in symbols:
        bucket, subgroup = classify(raw, demangled.get(raw, raw))
        key = (bucket, subgroup)
        info = groups.setdefault(key, [0, 0, (0, "")])  # sum, count, (max, name)
        info[0] += size
        info[1] += 1
        if size > info[2][0]:
            info[2] = (size, raw)

    bucket_order = ["singletons", "static members", "function-locals",
                    "file-locals", "runtime/library", "other"]

    def bucket_total(bucket):
        return sum(v[0] for (b, _), v in groups.items() if b == bucket)

    # ---- thresholds ----
    if args.all:
        threshold = 0
    else:
        threshold = total * args.min_percent / 100.0
        if args.min_bytes is not None:
            threshold = max(threshold, args.min_bytes)

    # ---- header ----
    print("ELF: {}".format(args.elf))
    print("Total static RAM footprint: {} ({} bytes, {} symbols)".format(
        human(total), total, len(symbols)))
    if args.all:
        print("Showing: every group (--all)")
    else:
        print("Showing: groups >= {} ({}% of total{}), by symbol kind".format(
            human(threshold), args.min_percent,
            ", or {} B".format(args.min_bytes) if args.min_bytes is not None else ""))
    print()

    # ---- summary bar per bucket ----
    print("RAM by kind  {:>7}  {:>8}  {:>6}".format("Symbols", "Bytes", "Share"))
    print("-" * 40)
    for bucket in sorted(bucket_order, key=bucket_total, reverse=True):
        btotal = bucket_total(bucket)
        if btotal < threshold:
            continue
        bsyms = sum(v[1] for (b, _), v in groups.items() if b == bucket)
        share = 100.0 * btotal / total if total else 0.0
        print("{:<13} {:>7}  {:>8}  {:>5.1f}%".format(
            bucket, bsyms, human(btotal), share))
    print()

    # ---- per-bucket subgroup tables ----
    for bucket in bucket_order:
        subs = [(sub, info) for (b, sub), info in groups.items() if b == bucket]
        if not subs:
            continue
        subs.sort(key=lambda kv: kv[1][0], reverse=True)
        shown = [(s, i) for s, i in subs if i[0] >= threshold] or [subs[0]]

        sub_w = max(len(s) for s, _ in shown)
        sub_w = min(sub_w, 46)
        print(bucket.upper())
        print("  {:<{}} {:>5}  {:>8}  Largest".format("Subgroup", sub_w, "Sym", "Bytes"))
        for sub, (bsize, bcount, (bmax, bmaxname)) in shown:
            largest = bmaxname
            if len(largest) > 48:
                largest = largest[:45] + "..."
            print("  {:<{}} {:>5}  {:>8}  {}".format(
                sub, sub_w, bcount, human(bsize), largest))
        print()

    print("Note: sizes reflect static allocation (.data + .bss) and code "
          "copied to RAM;")
    print("      runtime heap is not included. Kinds are a heuristic grouping "
          "of nm symbols.")


if __name__ == "__main__":
    main()
