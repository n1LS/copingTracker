"""In-memory data model for the converter.

This module is deliberately independent from both the SF2 binary layout
(:mod:`sf2`) and the generated C++ representation (:mod:`emit_cpp`): it is
the shared vocabulary the rest of the pipeline is built around.
"""

from __future__ import annotations

import enum
from dataclasses import dataclass, field
from typing import List


class LoopMode(enum.IntEnum):
    """Mirrors the two values of ``SampleInstrumentLoopMode`` that the
    firmware's ``SampleEntry.flags`` low nibble can carry (see
    ``SEF_LOOP_MODE_MASK`` in ``SampleInstrument.h``). Only forward looping
    is supported; every other SF2 loop mode is downgraded to ONESHOT with a
    warning.
    """

    ONESHOT = 0
    LOOP = 1


RUNTIME_SAMPLE_RATE_HZ = 22050
SOURCE_SAMPLE_RATE_HZ = 44100

MELODIC_INSTRUMENT_COUNT = 128
PERCUSSION_BANK = 128


@dataclass
class RawSample:
    """A single finalized, mono, 22050 Hz, 16-bit PCM sample instance ready
    to be deduplicated and emitted. Two regions that end up with identical
    field values will be collapsed into a single ``SampleEntry`` by
    :func:`optimize.deduplicate`.
    """

    name: str
    pcm: bytes  # little-endian signed 16-bit samples
    loop_start: int
    loop_end: int
    root_note: int
    fine_tune: int  # signed, matches SampleEntry.fineTune range [-128, 127]
    loop_mode: LoopMode

    @property
    def frame_count(self) -> int:
        return len(self.pcm) // 2


@dataclass
class ResolvedRegion:
    """One (key_low..key_high) -> sample mapping, fully resolved (i.e. all
    pitch/loop generators already combined), scoped to a single instrument.
    """

    key_low: int
    key_high: int
    sample_index: int  # index into the working RawSample list


@dataclass
class InstrumentDef:
    name: str
    regions: List[ResolvedRegion] = field(default_factory=list)


@dataclass
class PresetZone:
    key_low: int
    key_high: int
    instrument_index: int


@dataclass
class PresetDef:
    name: str
    bank: int
    preset: int
    zones: List[PresetZone] = field(default_factory=list)


@dataclass
class SampleEntryOut:
    """Mirrors the firmware's ``SampleEntry`` struct field-for-field."""

    pcm_offset: int
    length: int
    loop_start: int
    loop_end: int
    root_note: int
    fine_tune: int
    flags: int


@dataclass
class PresetInfoOut:
    name: str
    instrument_index: int  # 0-127 GM melodic, 128+ percussion
    bank: int
    preset: int


@dataclass
class GMBankData:
    """Everything required to emit ``GMBank_data.generated.h``."""

    sample_entries: List[SampleEntryOut]
    pcm_data: bytes
    lookup: List[List[int]]  # [instrument][note] -> index into sample_entries
    presets: List[PresetInfoOut]
    instrument_count: int
