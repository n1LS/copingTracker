#!/usr/bin/env bash
set -euo pipefail

: "${PICO_TOOLCHAIN_FILE:?Set PICO_TOOLCHAIN_FILE to your CMake toolchain file}"
: "${GM_SF2_FILE:?Set GM_SF2_FILE to your GeneralMidi sf2 file}"

quick=false
pretools=false
bootloader=false
minimal_gm=false

for arg in "$@"; do
    case "$arg" in
        quick) quick=true ;;
        pre) pretools=true ;;
        bootloader) bootloader=true ;;
        minimal_gm) minimal_gm=true ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

if [ "$quick" = false ]; then
    echo "Prebuild steps…"
    echo "1) Generating the font data"
    cd tools/fonts
    python3 import.py
    python3 font_bootloader.py > ../../sources/Adapters/copingTracker/bootloader/bl_font.generated.h
    cd ../..
    echo "2) Converting the documentation"
    python3 ./tools/manual/raw_data/convert-documentation.py ./tools/manual/raw_data/Documentation.rc sources/Foundation/Constants/Documentation.generated.h
    echo "3) Generating the GMBank data"
    cd tools/sf2converter
    GM_FLAG=""
    if [ "$minimal_gm" = true ]; then
        GM_FLAG="--minimal-gm"
    fi
    python3 -m ctsb_converter $GM_FLAG "$GM_SF2_FILE" ../../sources/Application/Instruments
    cd ../..
    echo "4) Formatting source code…"
    ./format.sh 
    echo "5) Generating stack wavetables…"
    python3 ./tools/wavetable_generator/wavetable_generator.py sources/Application/Instruments/StackInstrument/StackWavetables.generated.h

    if [ "$pretools" = true ]; then
        exit 0
    fi

    if [ "$bootloader" = true ]; then
        echo "Building in bootloader mode..."
    else
        echo "Building in device mode..."
    fi
    cmake -S sources -B build -DPICO_SDK_PATH=$PWD/sources/Externals/pico-sdk -DCMAKE_TOOLCHAIN_FILE="$PICO_TOOLCHAIN_FILE"
fi

if [ "$bootloader" = true ]; then
    cmake --build build --target PatchBay -j8
    picotool load ./build/Adapters/copingTracker/bootloader/PatchBay.uf2 && picotool reboot
else
    cmake --build build -j8
    cp build/Adapters/copingTracker/main/copingTracker.uf2 build/Adapters/copingTracker/main/copingTracker.patched.uf2
    python3 tools/uf2tools/patch_boot2_uf2.py build/Adapters/copingTracker/main/copingTracker.patched.uf2
    picotool load ./build/Adapters/copingTracker/main/copingTracker.patched.uf2 && picotool reboot
fi