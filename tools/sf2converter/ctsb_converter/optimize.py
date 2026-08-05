"""Post-processing steps applied to the finalized sample instance list:
unused-sample removal and exact-match deduplication.
"""

from __future__ import annotations

from typing import Dict, List, Tuple

from .model import RawSample


def deduplicate(samples: List[RawSample]) -> Tuple[List[RawSample], Dict[int, int]]:
    """Collapses samples that are identical in every field the firmware
    cares about (PCM, loop points, root note, fine tune, flags) into a
    single entry. Returns (unique_samples, old_index -> new_index map).

    The dummy silent sample must be the first element of ``samples`` so it
    deterministically ends up at index 0.
    """
    seen: Dict[tuple, int] = {}
    unique: List[RawSample] = []
    remap: Dict[int, int] = {}

    for old_index, sample in enumerate(samples):
        key = (sample.pcm, sample.loop_start, sample.loop_end, sample.root_note, sample.fine_tune, int(sample.loop_mode))
        existing = seen.get(key)
        if existing is not None:
            remap[old_index] = existing
            continue
        new_index = len(unique)
        seen[key] = new_index
        unique.append(sample)
        remap[old_index] = new_index

    return unique, remap
