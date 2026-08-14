#!/usr/bin/env python3

import re
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <wavetable-header.h>")
    sys.exit(1)

input_file = Path(sys.argv[1])
output_dir = input_file.parent / "wavetable_plots"
output_dir.mkdir(exist_ok=True)

text = input_file.read_text()


pattern = re.compile(
    r'alignas\s*\(\s*4\s*\)\s*'
    r'static\s+constexpr\s+int16_t\s+'
    r'([A-Za-z_]\w*)\s*\[\s*2048\s*\]\s*=\s*\{(.*?)\};',
    re.DOTALL,
)

matches = pattern.findall(text)

print(f"Found {len(matches)} wavetables")

for name, body in matches:
    values = np.array(
        [int(x) for x in re.findall(r'[-+]?\d+', body)],
        dtype=np.int16,
    )

    if len(values) != 2048:
        print(f"WARNING: {name}: found {len(values)} samples, skipping")
        continue

    fig, ax = plt.subplots(figsize=(20.48, 20.48), dpi=100)

    ax.plot(
        np.arange(2048),
        values,
        linewidth=1.0,
    )

    ax.set_title(name)
    ax.set_xlim(0, 2047)
    ax.set_ylim(-32768, 32767)

    ax.set_xlabel("Sample")
    ax.set_ylabel("Amplitude")

    ax.axhline(0, linewidth=0.5)
    ax.grid(True, alpha=0.25)

    fig.tight_layout()

    output_file = output_dir / f"{name}.png"
    fig.savefig(output_file, dpi=100)
    plt.close(fig)

    print(f"  {output_file}")

print(f"\nOutput: {output_dir}")