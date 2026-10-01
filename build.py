#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
ROOT = Path(__file__).resolve().parent
SDK = ROOT / "sources/Externals/pico-sdk"

def load_env():
    path = ROOT / ".env"
    if not path.exists():
        return
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip().strip("\"'")
        os.environ.setdefault(key.strip(), value)

def run(cmd, cwd=ROOT):
    subprocess.run(cmd, cwd=cwd, check=True)

def step(name, cmd, cwd=ROOT, ignore_error=False):
    print(f"  {name}...", end="", flush=True)
    start = time.monotonic()
    result = subprocess.run(
        cmd,
        cwd=cwd,
        check=False,
    )
    elapsed = time.monotonic() - start
    if result.returncode and not ignore_error:
        print(f"🛑  EXECUTION FAILED ({elapsed:.1f}s)")
        exit(1)
    print(f" {'FAILED' if result.returncode else 'OK'} ({elapsed:.1f}s)")

def prebuild(args):
    print("\nPrebuild")
    fonts = ROOT / "tools/fonts"
    boot_font = ROOT / "sources/Adapters/copingTracker/bootloader/bl_font.generated.h"
    step("Generating font data", [sys.executable, "import.py"], fonts)
    with boot_font.open("w") as f:
        subprocess.run(
            [sys.executable, "font_bootloader.py"],
            cwd=fonts,
            stdout=f,
            check=True,
        )
    step(
        "Updating changelog",
        [
            sys.executable,
            "tools/manual/update-changelog.py",
            "TODO.md",
            "tools/manual/raw_data/changelog.copingDoc",
            "sources/Foundation/Constants/Version.h",
        ],
    )
    gm = [sys.executable, "-m", "ctsb_converter"]
    if args.minimal_gm:
        gm.append("--minimal-gm")
    gm += [
        os.environ["GM_SF2_FILE"],
        "../../sources/Application/Instruments",
    ]
    step("Generating GM bank", gm, ROOT / "tools/sf2converter")
    step("Formatting source", ["bash", "format.sh"], ignore_error=True)
    step(
        "Generating stack wavetables",
        [
            sys.executable,
            "tools/wavetable_generator/wavetable_generator.py",
            "sources/Application/Instruments/StackInstrument/StackWavetables.generated.h",
        ],
    )
    step(
        "Converting documentation",
        [
            sys.executable,
            "tools/manual/raw_data/convert-documentation.py",
            "tools/manual/raw_data/Documentation.rc",
            "sources/Foundation/Constants/Documentation.generated.h",
        ],
    )
    step(
        "Generating command help",
        [
            sys.executable,
            "tools/manual/raw_data/convert-commandhelp.py",
            "tools/manual/raw_data/Commands.copingDoc",
            "sources/Application/Utils/CommandHelp.generated.h",
        ],
    )
    if args.manual:
        step(
            "Building HTML manual",
            [
                sys.executable,
                "tools/manual/webapp/build_webapp.py",
            ],
        )
    # TODO nILS: reenable once the wave is done and the converter is updated
    # step(
    #    "Export LSDJeque Drum Kits",
    #    [
    #        sys.executable,
    #        "tools/lsdj_kits/split_kit.py",
    #        os.environ["LSDJ_GB_FILE"],
    #        "-o",
    #        "sources/Application/Instruments/LSDJKitInstrument/LSDJKits.generated.h",
    #    ],
    #)

def main():
    load_env()
    parser = argparse.ArgumentParser()
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--pre", action="store_true")
    parser.add_argument("--bootloader", action="store_true")
    parser.add_argument("--host", action="store_true")
    parser.add_argument("--minimal-gm", action="store_true")
    parser.add_argument(
        "--manual",
        action="store_true",
        help="also regenerate tools/manual/webapp/index.html",
    )
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("-j", "--jobs", type=int, default=8)
    args = parser.parse_args()
    if args.host and args.bootloader:
        parser.error("--host and --bootloader are mutually exclusive")
    start = time.monotonic()
    try:
        if not args.quick:
            if not os.environ.get("LSDJ_GB_FILE"):
                parser.error("LSDJ_GB_FILE is required")
            if not os.environ.get("GM_SF2_FILE"):
                parser.error("GM_SF2_FILE is required")
            prebuild(args)
        if args.pre:
            return 0
        if args.host:
            if not args.quick:
                step(
                    "Configuring host",
                    [
                        "cmake",
                        "-S", "sources/Adapters/Host",
                        "-B", "build-host",
                        "-DCMAKE_C_FLAGS=-w",
                        "-DCMAKE_CXX_FLAGS=-w",
                    ],
                )
            step(
                "Building host",
                ["cmake", "--build", "build-host", f"-j{args.jobs}"],
            )
            if not args.build_only:
                step(
                    "Running host",
                    ["./build-host/main/copingTracker"],
                )
        else:
            toolchain = os.environ.get("PICO_TOOLCHAIN_FILE")
            if not args.quick and not toolchain:
                parser.error("PICO_TOOLCHAIN_FILE is required")
            if not args.quick:
                step(
                    "Configuring device",
                    [
                        "cmake",
                        "-S", "sources",
                        "-B", "build",
                        f"-DPICO_SDK_PATH={SDK}",
                        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
                    ],
                )
            target = "PatchBay" if args.bootloader else None
            build = ["cmake", "--build", "build"]
            if target:
                build += ["--target", target]
            build += [f"-j{args.jobs}"]
            step(
                "Building bootloader" if args.bootloader else "Building firmware",
                build,
            )
            if not args.build_only:
                if args.bootloader:
                    uf2 = "build/Adapters/copingTracker/bootloader/PatchBay.uf2"
                else:
                    uf2 = "build/Adapters/copingTracker/main/copingTracker.uf2"
                    patched = ROOT / "build/Adapters/copingTracker/main/copingTracker.patched.uf2"
                    shutil.copyfile(ROOT / uf2, patched)
                    step(
                        "Patching boot2",
                        [
                            sys.executable,
                            "tools/uf2tools/patch_boot2_uf2.py",
                            str(patched),
                        ],
                    )
                    uf2 = str(patched.relative_to(ROOT))
                step("Uploading", ["picotool", "load", uf2])
                step("Rebooting", ["picotool", "reboot"])
        print(f"\n✔️  Complete ({time.monotonic() - start:.1f}s)")
    except subprocess.CalledProcessError as e:
        print(f"\n❌  Command failed ({e.returncode})", file=sys.stderr)
        return e.returncode

if __name__ == "__main__":
    sys.exit(main())