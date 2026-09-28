#!/usr/bin/env python3

import argparse
import struct
import sys
import wave
from pathlib import Path


SAMPLE_RATE = 44100
CHANNELS = 1
BITS_PER_SAMPLE = 8
SLOTS_PER_BAR = 16

# hit_8 and hit_9 are quarter-note samples.
# Each consumes two eighth-note timing slots.
LONG_SAMPLE_INDICES = {8, 9}


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Convert a 44.1kHz/8-bit mono drum WAV into a C header. "
            "The number of kits is determined automatically from the WAV length."
        )
    )

    parser.add_argument(
        "--bpm",
        type=float,
        required=True,
        help="Tempo of the source WAV in BPM (e.g. 80)",
    )

    parser.add_argument(
        "--threshold",
        type=int,
        default=0,
        help="Absolute PCM amplitude treated as silence (default: 0)",
    )

    parser.add_argument(
        "input",
        type=Path,
        help="Input WAV file",
    )

    parser.add_argument(
        "output",
        type=Path,
        help="Output C header file",
    )

    args = parser.parse_args()

    if args.bpm <= 0:
        parser.error("--bpm must be greater than zero")

    if not 0 <= args.threshold <= 128:
        parser.error("--threshold must be between 0 and 128")

    return args


def read_wav(path):
    try:
        with wave.open(str(path), "rb") as wav:
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
            sample_rate = wav.getframerate()
            frame_count = wav.getnframes()
            compression = wav.getcomptype()

            if channels != CHANNELS:
                raise ValueError(
                    f"expected mono WAV, got {channels} channels"
                )

            if sample_width != 1:
                raise ValueError(
                    f"expected 8-bit WAV, got {sample_width * 8}-bit"
                )

            if sample_rate != SAMPLE_RATE:
                raise ValueError(
                    f"expected {SAMPLE_RATE} Hz, got {sample_rate} Hz"
                )

            if compression != "NONE":
                raise ValueError(
                    f"expected uncompressed PCM, got {compression}"
                )

            raw = wav.readframes(frame_count)

    except wave.Error as exc:
        raise ValueError(f"invalid WAV file: {exc}") from exc

    # 8-bit PCM WAV is unsigned: 0..255.
    # Convert to signed -128..127 for the generated C data.
    return [
        sample - 128
        for sample in raw
    ]


def samples_per_eighth(bpm):
    return SAMPLE_RATE * 60.0 / bpm / 2.0


def determine_num_kits(num_samples, bpm):
    """
    Determine the number of complete 16-slot kits/bars in the WAV.

    Extra audio that does not make up a complete kit is rejected rather
    than silently discarded.
    """
    interval = samples_per_eighth(bpm)
    samples_per_bar = interval * SLOTS_PER_BAR

    num_kits = round(num_samples / samples_per_bar)

    if num_kits < 1:
        raise ValueError(
            "WAV is too short: it does not contain one complete kit/bar"
        )

    expected_samples = round(num_kits * samples_per_bar)

    # Allow a one-sample rounding difference caused by fractional
    # sample positions.
    tolerance = 1

    if abs(num_samples - expected_samples) > tolerance:
        duration = num_samples / SAMPLE_RATE
        expected_duration = expected_samples / SAMPLE_RATE

        raise ValueError(
            f"WAV length does not contain a whole number of kits: "
            f"got {duration:.3f} seconds, "
            f"expected approximately {expected_duration:.3f} seconds "
            f"for {num_kits} kits"
        )

    return num_kits


def make_grid(num_kits, bpm):
    """
    Return sample positions for all eighth-note boundaries.

    There are 16 eighth-note slots per kit/bar.

    The exact fractional sample position is calculated from the beginning
    rather than repeatedly adding a rounded interval, preventing cumulative
    timing drift.
    """
    total_slots = num_kits * SLOTS_PER_BAR
    interval = samples_per_eighth(bpm)

    return [
        round(i * interval)
        for i in range(total_slots + 1)
    ]


def find_hit(samples, start, end, threshold):
    """
    Find the non-silent portion of one sample region.

    A hit is considered present when at least one sample has:
        abs(sample) > threshold

    The returned range excludes all leading and trailing silence.
    """
    first = None
    last = None

    for i in range(start, end):
        if abs(samples[i]) > threshold:
            if first is None:
                first = i
            last = i

    if first is None:
        return None

    return first, last + 1


def extract_kits(samples, bpm, threshold, num_kits):
    grid = make_grid(num_kits, bpm)

    expected_positions = num_kits * SLOTS_PER_BAR

    if len(grid) != expected_positions + 1:
        raise AssertionError("invalid grid")

    kits = []

    for kit_index in range(num_kits):
        kit_data = []
        hits = []

        # Output sample index.
        sample_index = 0

        # Timing slot within this kit.
        slot = 0

        while slot < SLOTS_PER_BAR:
            global_slot = kit_index * SLOTS_PER_BAR + slot

            # hit_8 and hit_9 each occupy two eighth-note slots.
            slot_length = (
                2
                if sample_index in LONG_SAMPLE_INDICES
                else 1
            )

            end_slot = min(
                slot + slot_length,
                SLOTS_PER_BAR,
            )

            start = grid[global_slot]
            end = grid[
                kit_index * SLOTS_PER_BAR + end_slot
            ]

            # Don't read beyond the actual WAV.
            start = min(start, len(samples))
            end = min(end, len(samples))

            if start < end:
                hit = find_hit(
                    samples,
                    start,
                    end,
                    threshold,
                )

                if hit is not None:
                    hit_start, hit_end = hit

                    data_position = len(kit_data)
                    hit_samples = samples[hit_start:hit_end]

                    kit_data.extend(hit_samples)

                    hits.append(
                        {
                            "name": f"hit_{sample_index}",
                            "position": data_position,
                            "length": len(hit_samples),
                        }
                    )

            slot = end_slot
            sample_index += 1

        kits.append(
            {
                "name": f"kit_{kit_index}",
                "data": kit_data,
                "hits": hits,
            }
        )

    return kits


def c_identifier(name):
    return "".join(
        char if char.isalnum() or char == "_" else "_"
        for char in name
    )


def format_int8_array(values, indent="    "):
    if not values:
        return f"{indent}0"

    lines = []

    for i in range(0, len(values), 16):
        chunk = values[i:i + 16]
        lines.append(
            indent + ", ".join(str(value) for value in chunk)
        )

    return ",\n".join(lines)


def generate_header(kits, bpm, threshold, input_name):
    lines = []

    lines.append("/*")
    lines.append(" * Generated by wav2h")
    lines.append(f" * Source: {input_name}")
    lines.append(f" * BPM: {bpm:g}")
    lines.append(f" * Silence threshold: {threshold}")
    lines.append(f" * Kits: {len(kits)}")
    lines.append(" *")
    lines.append(" * Input: 8-bit unsigned PCM WAV")
    lines.append(" * Output: signed int8_t PCM (-128..127)")
    lines.append(" * Each kit contains 16 eighth-note timing slots.")
    lines.append(" * hit_8 and hit_9 are quarter-note samples.")
    lines.append(" */")
    lines.append("")
    lines.append("#ifndef DRUM_KITS_H")
    lines.append("#define DRUM_KITS_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    const char *name;")
    lines.append("    uint32_t position;")
    lines.append("    uint32_t length;")
    lines.append("} Sample;")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    const char *name;")
    lines.append("    uint32_t num_samples;")
    lines.append("    const int8_t *data;")
    lines.append("    const Sample *samples;")
    lines.append("} Kit;")
    lines.append("")

    for kit in kits:
        name = c_identifier(kit["name"])
        data_name = f"{name}_data"
        samples_name = f"{name}_samples"

        lines.append(f"static const int8_t {data_name}[] = {{")
        lines.append(format_int8_array(kit["data"]))
        lines.append("};")
        lines.append("")

        lines.append(f"static const Sample {samples_name}[] = {{")

        if kit["hits"]:
            for hit in kit["hits"]:
                lines.append(
                    f'    {{ "{hit["name"]}", '
                    f'{hit["position"]}u, '
                    f'{hit["length"]}u }},'
                )
        else:
            lines.append("    { NULL, 0u, 0u },")

        lines.append("};")
        lines.append("")

    lines.append("static const Kit drum_kits[] = {")

    for kit in kits:
        name = c_identifier(kit["name"])
        data_name = f"{name}_data"
        samples_name = f"{name}_samples"

        lines.append(
            f'    {{ "{kit["name"]}", '
            f'{len(kit["hits"])}u, '
            f'{data_name}, '
            f'{samples_name} }},'
        )

    lines.append("};")
    lines.append("")
    lines.append(
        "static const uint32_t drum_kit_count = "
        f"{len(kits)}u;"
    )
    lines.append("")
    lines.append("#endif /* DRUM_KITS_H */")
    lines.append("")

    return "\n".join(lines)


def main():
    args = parse_args()

    try:
        samples = read_wav(args.input)

        num_kits = determine_num_kits(
            len(samples),
            args.bpm,
        )

        kits = extract_kits(
            samples,
            args.bpm,
            args.threshold,
            num_kits,
        )

        header = generate_header(
            kits,
            args.bpm,
            args.threshold,
            args.input.name,
        )

        args.output.write_text(
            header,
            encoding="utf-8",
            newline="\n",
        )

        total_hits = sum(
            len(kit["hits"])
            for kit in kits
        )

        total_samples = sum(
            len(kit["data"])
            for kit in kits
        )

        print(f"Input:       {args.input}")
        print(f"Output:      {args.output}")
        print(f"BPM:         {args.bpm:g}")
        print(f"Threshold:   {args.threshold}")
        print(f"Kits:        {len(kits)}")
        print(f"Max samples: 14 per kit")
        print(f"Hits:        {total_hits}")
        print(f"PCM samples: {total_samples}")

    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
