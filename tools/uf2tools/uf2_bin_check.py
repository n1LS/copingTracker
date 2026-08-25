#!/usr/bin/env python3

import struct
import sys


UF2_BLOCK_SIZE = 512
UF2_PAYLOAD_OFFSET = 32
UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30


def read_uf2(path):
    blocks = []

    with open(path, "rb") as f:
        while True:
            block = f.read(UF2_BLOCK_SIZE)

            if not block:
                break

            if len(block) != UF2_BLOCK_SIZE:
                raise ValueError("Truncated UF2 block")

            magic0, magic1 = struct.unpack_from("<II", block, 0)
            magic_end = struct.unpack_from("<I", block, 508)[0]

            if magic0 != UF2_MAGIC_START0 or \
               magic1 != UF2_MAGIC_START1 or \
               magic_end != UF2_MAGIC_END:
                continue

            target_addr = struct.unpack_from("<I", block, 12)[0]
            payload_size = struct.unpack_from("<I", block, 16)[0]

            payload = block[
                UF2_PAYLOAD_OFFSET:
                UF2_PAYLOAD_OFFSET + payload_size
            ]

            blocks.append((target_addr, payload))

    if not blocks:
        raise ValueError("No valid UF2 blocks found")

    return blocks


def compare(bin_path, uf2_path, target_slot):
    blocks = read_uf2(uf2_path)

    mismatches = 0
    checked = 0

    with open(bin_path, "rb") as f:
        bin_data = f.read()

    print(f"BIN size: {len(bin_data)} bytes")
    print(f"UF2 blocks: {len(blocks)}")
    print(f"BIN base:  0x{target_slot:08x}")
    print()

    for target_addr, payload in blocks:
        # Ignore anything before the application slot (e.g. boot2)
        if target_addr + len(payload) <= target_slot:
            continue

        # Handle a block crossing the slot boundary.
        payload_offset = 0
        if target_addr < target_slot:
            payload_offset = target_slot - target_addr
            target_addr = target_slot

        payload = payload[payload_offset:]

        bin_offset = target_addr - target_slot

        if bin_offset >= len(bin_data):
            print(
                f"UF2 data at 0x{target_addr:08x} is beyond BIN "
                f"(BIN offset 0x{bin_offset:x})"
            )
            mismatches += len(payload)
            continue

        available = min(len(payload), len(bin_data) - bin_offset)

        for i in range(available):
            uf2_byte = payload[i]
            bin_byte = bin_data[bin_offset + i]

            checked += 1

            if uf2_byte != bin_byte:
                if mismatches < 32:
                    print(
                        f"MISMATCH at flash 0x{target_addr + i:08x}: "
                        f"UF2={uf2_byte:02x} BIN={bin_byte:02x}"
                    )

                mismatches += 1

    print()
    print(f"Bytes checked: {checked}")
    print(f"Mismatches:    {mismatches}")

    if mismatches == 0:
        print("RESULT: BIN matches UF2 payloads.")
    else:
        print("RESULT: BIN DOES NOT match UF2 payloads.")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(
            f"Usage: {sys.argv[0]} <application.bin> <application.uf2> "
            f"<target_slot>"
        )
        print("Example:")
        print(
            f"  {sys.argv[0]} app.bin app.uf2 0x10000100"
        )
        sys.exit(1)

    target_slot = int(sys.argv[3], 0)

    compare(
        sys.argv[1],
        sys.argv[2],
        target_slot,
    )