#!/usr/bin/env bash
set -euo pipefail

: "${PICO_TOOLCHAIN_FILE:?Set PICO_TOOLCHAIN_FILE to your CMake toolchain file}"
: "${GM_SF2_FILE:?Set GM_SF2_FILE to your GeneralMidi sf2 file}"

quick=false
pretools=false
bootloader=false
minimal_gm=false
host=false

for arg in "$@"; do
    case "$arg" in
        quick) quick=true ;;
        pre) pretools=true ;;
        bootloader) bootloader=true ;;
        minimal_gm) minimal_gm=true ;;
        host) host=true ;;
        *) 
          echo "Unknown option: '$arg'. Available options are:"
          echo "  quick      (skip cmake -S and prebuild steps)"
          echo "  pre        (prebuild steps only)"
          echo "  bootloader (build bootloader target)"
          echo "  minimal_gm (do not write full gm set to keep the flash size down)"
          echo "  host       (build host target)"
          exit 2 
          ;;
    esac
done

if [ "$quick" = false ]; then
    echo "Prebuild steps…"
    echo "1) Generating the font data"
    cd tools/fonts
    python3 import.py
    python3 font_bootloader.py > ../../sources/Adapters/copingTracker/bootloader/bl_font.generated.h
    cd ../..
    echo "2) Updating the changelog"
    python3 ./tools/manual/update-changelog.py TODO.md tools/manual/raw_data/changelog.copingDoc sources/Foundation/Constants/Version.h
    echo "3) Generating the GMBank data"
    cd tools/sf2converter
    GM_FLAG=""
    if [ "$minimal_gm" = true ]; then
        GM_FLAG="--minimal-gm"
    fi
    python3 -m ctsb_converter $GM_FLAG "$GM_SF2_FILE" ../../sources/Application/Instruments
    cd ../..
    echo "4) Formatting source code…"
    ./format.sh || true
    echo "5) Generating stack wavetables…"
    python3 ./tools/wavetable_generator/wavetable_generator.py sources/Application/Instruments/StackInstrument/StackWavetables.generated.h
    echo "6) Converting the documentation"
    python3 ./tools/manual/raw_data/convert-documentation.py ./tools/manual/raw_data/Documentation.rc sources/Foundation/Constants/Documentation.generated.h

    if [ "$pretools" = true ]; then
        exit 0
    fi

    if [ "$bootloader" = true ]; then
        echo "Building in bootloader mode..."
    else
        echo "Building in device mode..."
    fi
    if [ "$host" = true ]; then
        echo "Preparing host build"
        cmake -S sources/Adapters/Host -B build-host -DCMAKE_C_FLAGS="-w" -DCMAKE_CXX_FLAGS="-w"
        echo "\nPrebuild steps are done.\n"
    else
        echo "Preparing pico-sdk build"
        cmake -S sources -B build -DPICO_SDK_PATH=$PWD/sources/Externals/pico-sdk -DCMAKE_TOOLCHAIN_FILE="$PICO_TOOLCHAIN_FILE"
    fi
fi

if [ "$host" = true ]; then
    cmake --build build-host -j8
    ./build-host/main/copingTracker
elif [ "$bootloader" = true ]; then
    cmake --build build --target PatchBay -j8
    picotool load ./build/Adapters/copingTracker/bootloader/PatchBay.uf2 && picotool reboot
else
    cmake --build build -j8
    cp build/Adapters/copingTracker/main/copingTracker.uf2 build/Adapters/copingTracker/main/copingTracker.patched.uf2
    python3 tools/uf2tools/patch_boot2_uf2.py build/Adapters/copingTracker/main/copingTracker.patched.uf2
    picotool load ./build/Adapters/copingTracker/main/copingTracker.patched.uf2 && picotool reboot
fi