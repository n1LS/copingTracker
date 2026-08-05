"""Audio-level processing: mono downmixing, 44.1kHz -> 22.05kHz decimation
and optional silence trimming.

Pure functions operating on plain ``list[int]`` sample buffers - no SF2 or
C++ knowledge lives here.
"""

from __future__ import annotations

import struct
from typing import List, Tuple

from .model import RUNTIME_SAMPLE_RATE_HZ, SOURCE_SAMPLE_RATE_HZ
from .util import Warnings

SAMPLE_TYPE_ROM_FLAG = 0x8000
SAMPLE_TYPE_MONO = 1
SAMPLE_TYPE_RIGHT = 2
SAMPLE_TYPE_LEFT = 4
SAMPLE_TYPE_LINKED = 8

DEFAULT_SILENCE_THRESHOLD = 32  # ~ -60 dBFS relative to a 16-bit full-scale


def pcm_to_samples(pcm: bytes) -> List[int]:
    count = len(pcm) // 2
    if count == 0:
        return []
    return list(struct.unpack(f"<{count}h", pcm[: count * 2]))


def samples_to_pcm(samples: List[int]) -> bytes:
    if not samples:
        return b""
    return struct.pack(f"<{len(samples)}h", *samples)


def merge_stereo(left: List[int], right: List[int]) -> List[int]:
    """Average matching left/right samples into mono, truncating to the
    shorter channel if lengths differ (mismatched stereo pairs are unusual
    but not invalid SF2)."""
    n = min(len(left), len(right))
    return [(left[i] + right[i]) // 2 for i in range(n)]


def downsample_44100_to_22050(samples: List[int]) -> List[int]:
    """Simple 2:1 decimation (pairwise average, a mild low-pass filter)."""
    out: List[int] = []
    n = len(samples)
    i = 0
    while i + 1 < n:
        out.append((samples[i] + samples[i + 1]) // 2)
        i += 2
    if i < n:
        out.append(samples[i])
    return out


def convert_sample_rate(
    samples: List[int], sample_rate: int, name: str, warnings: Warnings
) -> Tuple[List[int], int, bool]:
    """Returns (samples, scale_factor, ok). ``scale_factor`` is the integer
    divisor that must also be applied to any pre-computed (pre-conversion)
    sample-unit offset, such as loop points. ``ok`` is False when the sample
    rate is unsupported and the sample should be skipped entirely."""
    if sample_rate == RUNTIME_SAMPLE_RATE_HZ:
        return samples, 1, True
    if sample_rate == SOURCE_SAMPLE_RATE_HZ:
        return downsample_44100_to_22050(samples), 2, True
    warnings.add(f"sample '{name}': unsupported sample rate {sample_rate} Hz, skipping")
    return samples, 1, False


def trim_silence(
    samples: List[int],
    loop_start: int,
    loop_end: int,
    has_loop: bool,
    threshold: int = DEFAULT_SILENCE_THRESHOLD,
) -> Tuple[List[int], int, int, int]:
    """Trims leading/trailing near-silence, never trimming into the loop
    region of a looping sample. Returns (samples, loop_start, loop_end,
    trimmed_from_start)."""
    n = len(samples)
    if n == 0:
        return samples, loop_start, loop_end, 0

    start = 0
    limit_start = loop_start if has_loop else n
    limit_start = max(0, min(limit_start, n))
    while start < limit_start and abs(samples[start]) <= threshold:
        start += 1

    end = n
    limit_end = loop_end if has_loop else 0
    limit_end = max(0, min(limit_end, n))
    while end > limit_end and end > start and abs(samples[end - 1]) <= threshold:
        end -= 1

    trimmed = samples[start:end]
    if has_loop:
        new_loop_start = max(0, loop_start - start)
        new_loop_end = max(0, loop_end - start)
    else:
        new_loop_start = 0
        new_loop_end = 0
    return trimmed, new_loop_start, new_loop_end, start
