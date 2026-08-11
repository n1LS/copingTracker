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


def _downsample_44100_to_22050(samples: List[int]) -> List[int]:
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


def resample(samples: List[int], source_rate: int, target_rate: int) -> List[int]:
    """Resample from *source_rate* Hz to *target_rate* Hz using linear
    interpolation. Both rates must be positive integers.

    The ratio ``target_rate / source_rate`` determines the output length; the
    very last input sample is faithfully duplicated so the output never drops
    trailing values at near-unity ratios.
    """
    if source_rate == target_rate:
        return samples[:]
    if not samples:
        return []

    ratio = target_rate / source_rate
    out_len = int(round(len(samples) * ratio))
    if out_len < 1:
        return [samples[0]]

    out: List[int] = []
    max_src_index = len(samples) - 1
    for i in range(out_len):
        src_pos = i / ratio
        src_idx = int(src_pos)
        frac = src_pos - src_idx
        if src_idx >= max_src_index:
            out.append(samples[max_src_index])
        else:
            a = samples[src_idx]
            b = samples[src_idx + 1]
            out.append(int(round(a + (b - a) * frac)))
    return out


def convert_sample_rate(
    samples: List[int], sample_rate: int, name: str, warnings: Warnings
) -> Tuple[List[int], float, bool]:
    """Returns (samples, scale_factor, ok).

    *scale_factor* is a float that must be divided into any pre-resampling
    sample-unit offset (e.g. loop points).

    When the source rate is an exact integer submultiple of the target
    (e.g. 11025 → 22050) the PCM is returned unchanged and *scale_factor* is
    1.0 — the caller should compensate by shifting the sample's root note down
    by the corresponding number of semitones (12 per octave).

    *ok* is always ``True``: every sample rate is supported via either
    pass-through (integer submultiple), pairwise decimation (44100 Hz) or
    general linear-interpolation resampling (everything else).
    """
    if sample_rate == RUNTIME_SAMPLE_RATE_HZ:
        # Already at the target rate — nothing to do.
        return samples, 1.0, True

    if RUNTIME_SAMPLE_RATE_HZ % sample_rate == 0:
        # Integer submultiple: the sample plays back at a higher pitch
        # naturally when run at the target rate.  Return unchanged PCM;
        # the caller adjusts root note by:
        #   semitones = 12 * log2(RUNTIME_SAMPLE_RATE_HZ / sample_rate)
        return samples, 1.0, True

    if sample_rate == SOURCE_SAMPLE_RATE_HZ:
        # 44100 → 22050: pairwise average (acts as a simple low-pass).
        return _downsample_44100_to_22050(samples), 2.0, True

    # General case: linear-interpolation resampling.
    return resample(samples, sample_rate, RUNTIME_SAMPLE_RATE_HZ), sample_rate / RUNTIME_SAMPLE_RATE_HZ, True


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
