#!/usr/bin/env python3

import struct
import sys

UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_BLOCK_SIZE = 512
UF2_DATA_SIZE = 476
BOOT2_ADDRESS = 0x10000000
BOOT2_SIZE = 256

def extract_boot2(uf2_path, output_path):
    with open(uf2_path, "rb") as f:
        while True:
            block = f.read(UF2_BLOCK_SIZE)

            if not block:
                break

            if len(block) != UF2_BLOCK_SIZE:
                raise RuntimeError("Incomplete UF2 block")

            magic0, magic1 = struct.unpack_from("<II", block, 0)

            if magic0 != UF2_MAGIC0 or magic1 != UF2_MAGIC1:
                raise RuntimeError("Not a valid UF2 file")
#!/usr/bin/env python3

import struct
import sys

UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_BLOCK_SIZE = 512
BOOT2_ADDRESS = 0x10000000
BOOT2_SIZE = 256


def extract_boot2(uf2_path):
    with open(uf2_path, "rb") as f:
        while True:
            block = f.read(UF2_BLOCK_SIZE)

            if not block:
                break

            if len(block) != UF2_BLOCK_SIZE:
                raise RuntimeError("Incomplete UF2 block")

            magic0, magic1 = struct.unpack_from("<II", block, 0)

            if magic0 != UF2_MAGIC0 or magic1 != UF2_MAGIC1:
                raise RuntimeError("Invalid UF2 file")

            address = struct.unpack_from("<I", block, 12)[0]
            size = struct.unpack_from("<I", block, 16)[0]

            if address == BOOT2_ADDRESS:
                if size < BOOT2_SIZE:
                    raise RuntimeError("Boot2 block is too small")

                return block[32:32 + BOOT2_SIZE]

    raise RuntimeError("Could not find boot2 at 0x10000000")


def generate_patcher(boot2, output_path):
    boot2_source = ",\n".join(
        "    " + ", ".join(f"0x{x:02x}" for x in boot2[i:i + 16])
        for i in range(0, 256, 16)
    )

    script = f'''#!/usr/bin/env python3

import struct
import sys

UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_BLOCK_SIZE = 512
BOOT2_ADDRESS = 0x10000000
BOOT2_SIZE = 256

BOOT2 = bytes([
{boot2_source}
])


def patch_uf2(filename):
    with open(filename, "rb") as f:
        data = bytearray(f.read())

    if len(data) % UF2_BLOCK_SIZE != 0:
        raise RuntimeError("File size is not a multiple of 512 bytes")

    found = False

    for offset in range(0, len(data), UF2_BLOCK_SIZE):
        block = data[offset:offset + UF2_BLOCK_SIZE]

        magic0, magic1 = struct.unpack_from("<II", block, 0)

        if magic0 != UF2_MAGIC0 or magic1 != UF2_MAGIC1:
            raise RuntimeError(f"Invalid UF2 block at offset 0x{{offset:x}}")

        address = struct.unpack_from("<I", block, 12)[0]
        size = struct.unpack_from("<I", block, 16)[0]

        if address == BOOT2_ADDRESS:
            if size < BOOT2_SIZE:
                raise RuntimeError("Boot2 block is too small")

            data[offset + 32:offset + 32 + BOOT2_SIZE] = BOOT2
            found = True
            break

    if not found:
        raise RuntimeError("Could not find boot2 at 0x10000000")

    with open(filename, "wb") as f:
        f.write(data)

    print(f"Patched {{filename}}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {{sys.argv[0]}} firmware.uf2")
        sys.exit(1)

    patch_uf2(sys.argv[1])
'''

    with open(output_path, "w") as f:
        f.write(script)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} PatchBay.uf2 (or other bootloader)")
        sys.exit(1)

    boot2 = extract_boot2(sys.argv[1])

    if len(boot2) != 256:
        raise RuntimeError("Extracted boot2 is not 256 bytes")

    generate_patcher(boot2, "patch_boot2_uf2.py")

    print(f"Extracted boot2 and generated patch_boot2_uf2.py")
