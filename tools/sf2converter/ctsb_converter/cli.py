"""Command line interface and top-level conversion pipeline.

Pipeline order (see module docstring of each stage for details):

  1. Parse RIFF                         -> riff.parse_riff
  2. Parse SF2 chunks                   -> sf2.parse_sf2
  3. Resolve presets/instruments/regions -> sf2.build_instruments/build_presets
  4. Resolve every instrument/note into one sample -> _resolve_lookup
  5. Convert stereo to mono             -> audio.merge_stereo
  6. Downsample 44100 Hz -> 22050 Hz    -> audio.convert_sample_rate
  7. Adjust loop points                 -> _get_or_create_variant
  8. Trim silence (optional)            -> audio.trim_silence
  9. Remove unused samples (optional)   -> _add_orphan_variants (skipped if enabled)
 10. Deduplicate identical samples      -> optimize.deduplicate
 11. Build contiguous PCM blob          -> _build_pcm_blob
 12. Generate SampleEntry table         -> _build_sample_entries
 13. Generate lookup table              -> _resolve_lookup
 14. Generate preset metadata           -> _build_preset_infos
 15. Write GMBank_data.generated.h          -> emit_cpp.write_header
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from . import audio, emit_cpp, optimize, sf2
from .model import (
    GMBankData,
    InstrumentDef,
    LoopMode,
    MELODIC_INSTRUMENT_COUNT,
    PERCUSSION_BANK,
    PresetInfoOut,
    RawSample,
    ResolvedRegion,
    SampleEntryOut,
)
from .util import Logger, Warnings, clamp, parse_size

_DUMMY_SAMPLE_FRAMES = 32
_DEFAULT_SAMPLE_MODES_NO_LOOP = 0
_LOOP_SAMPLE_MODES = {1, 3}
# Sentinel lookup-table value meaning "no sample region covers this note",
# matching GMBank::sampleForNote()'s 0xffff check. Note that sample index 0
# is *not* usable as a "no coverage" default: it always legitimately refers
# to the dummy silent sample (see _dummy_sample()), so a real note with no
# coverage must be distinguishable from one that happens to resolve to
# index 0.
_NO_SAMPLE_INDEX = 0xFFFF


def _parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="ctsb_converter",
        description="Convert a SoundFont 2 (.sf2) file into GMBank_data.generated.h for the copingTracker firmware.",
    )
    parser.add_argument("input", type=Path, help="path to the input .sf2 file")
    parser.add_argument("output_directory", type=Path, help="directory to write GMBank_data.generated.h into")
    parser.add_argument("--mono", action="store_true", help="(accepted for compatibility; output is always mono)")
    parser.add_argument("--trim-silence", action="store_true", help="trim leading/trailing silence from samples")
    parser.add_argument("--remove-unused", action="store_true", help="drop samples not referenced by any preset")
    parser.add_argument("--verbose", action="store_true", help="print progress information to stderr")
    parser.add_argument("--max-size", type=parse_size, default=None, help="warn if the PCM blob exceeds this many bytes")
    return parser.parse_args(argv)


def _dummy_sample() -> RawSample:
    return RawSample(
        name="<dummy silence>",
        pcm=audio.samples_to_pcm([0] * _DUMMY_SAMPLE_FRAMES),
        loop_start=0,
        loop_end=0,
        root_note=60,
        fine_tune=0,
        loop_mode=LoopMode.ONESHOT,
    )


def _cents_to_fine_tune_units(cents: int) -> int:
    # 128 units span one semitone (100 cents), matching how the firmware's
    # existing SampleInstrument fine-tune variable is interpreted.
    return clamp(round(cents / 100.0 * 128), -128, 127)


class _ChannelAudioCache:
    """Lazily builds and memoizes the mono, sample-rate-converted PCM for
    each raw SF2 sample header (steps 5-6 of the pipeline)."""

    def __init__(self, sf2_data: sf2.Sf2Data, warnings: Warnings) -> None:
        self._sf2 = sf2_data
        self._warnings = warnings
        self._cache: Dict[int, Optional[Tuple[List[int], int]]] = {}

    def get(self, shdr_index: int) -> Optional[Tuple[List[int], int]]:
        if shdr_index in self._cache:
            return self._cache[shdr_index]
        result = self._build(shdr_index)
        self._cache[shdr_index] = result
        return result

    def _slice(self, start: int, end: int) -> List[int]:
        return audio.pcm_to_samples(self._sf2.pcm[start * 2 : end * 2])

    def _build(self, shdr_index: int) -> Optional[Tuple[List[int], int]]:
        shdr = self._sf2.samples[shdr_index]

        if shdr.sample_type & audio.SAMPLE_TYPE_ROM_FLAG:
            self._warnings.add(f"sample '{shdr.name}': ROM samples are not supported, skipping")
            return None

        type_bits = shdr.sample_type & ~audio.SAMPLE_TYPE_ROM_FLAG
        own = self._slice(shdr.start, shdr.end)

        if type_bits == audio.SAMPLE_TYPE_MONO:
            mono = own
        elif type_bits in (audio.SAMPLE_TYPE_LEFT, audio.SAMPLE_TYPE_RIGHT):
            link = shdr.sample_link
            if not (0 <= link < len(self._sf2.samples)):
                self._warnings.add(f"sample '{shdr.name}': invalid stereo link, treating as mono")
                mono = own
            else:
                partner = self._sf2.samples[link]
                partner_samples = self._slice(partner.start, partner.end)
                mono = (
                    audio.merge_stereo(own, partner_samples)
                    if type_bits == audio.SAMPLE_TYPE_LEFT
                    else audio.merge_stereo(partner_samples, own)
                )
        else:
            self._warnings.add(f"sample '{shdr.name}': unsupported sample type {shdr.sample_type}, skipping")
            return None

        converted, scale, ok = audio.convert_sample_rate(mono, shdr.sample_rate, shdr.name, self._warnings)
        if not ok:
            return None
        return converted, scale


class _VariantBuilder:
    """Builds finalized :class:`RawSample` instances (steps 7-8) and
    memoizes identical requests."""

    def __init__(self, sf2_data: sf2.Sf2Data, channels: _ChannelAudioCache, trim_silence: bool, warnings: Warnings) -> None:
        self._sf2 = sf2_data
        self._channels = channels
        self._trim_silence = trim_silence
        self._warnings = warnings
        self.instances: List[RawSample] = [_dummy_sample()]
        self._cache: Dict[tuple, int] = {}

    def get_or_create(
        self,
        shdr_index: int,
        root_note: int,
        fine_tune_cents: int,
        loop_start_offset: int,
        loop_end_offset: int,
        sample_modes: int,
    ) -> Optional[int]:
        channel = self._channels.get(shdr_index)
        if channel is None:
            return None
        samples, scale = channel
        shdr = self._sf2.samples[shdr_index]

        loop_start_rel = (shdr.loop_start + loop_start_offset) - shdr.start
        loop_end_rel = (shdr.loop_end + loop_end_offset) - shdr.start
        loop_start = clamp(loop_start_rel // scale, 0, len(samples))
        loop_end = clamp(loop_end_rel // scale, 0, len(samples))

        has_loop = sample_modes in _LOOP_SAMPLE_MODES
        if sample_modes not in (0, 1, 2, 3):
            self._warnings.add(f"sample '{shdr.name}': unknown sampleModes {sample_modes}, disabling loop")
            has_loop = False
        elif sample_modes == 2:
            self._warnings.add(f"sample '{shdr.name}': unsupported (reserved) loop mode, disabling loop")
            has_loop = False

        if has_loop and loop_start >= loop_end:
            self._warnings.add(f"sample '{shdr.name}': invalid loop points ({loop_start}, {loop_end}), disabling loop")
            has_loop = False

        if not has_loop:
            loop_start = 0
            loop_end = 0

        root_note = clamp(root_note, 0, 127)
        fine_tune = _cents_to_fine_tune_units(fine_tune_cents)
        loop_mode = LoopMode.LOOP if has_loop else LoopMode.ONESHOT

        cache_key = (shdr_index, loop_start, loop_end, loop_mode, root_note, fine_tune, self._trim_silence)
        cached = self._cache.get(cache_key)
        if cached is not None:
            return cached

        final_samples = samples
        final_loop_start, final_loop_end = loop_start, loop_end
        if self._trim_silence:
            final_samples, final_loop_start, final_loop_end, _ = audio.trim_silence(
                samples, loop_start, loop_end, has_loop
            )

        index = len(self.instances)
        self.instances.append(
            RawSample(
                name=shdr.name,
                pcm=audio.samples_to_pcm(final_samples),
                loop_start=final_loop_start,
                loop_end=final_loop_end,
                root_note=root_note,
                fine_tune=fine_tune,
                loop_mode=loop_mode,
            )
        )
        self._cache[cache_key] = index
        return index


def _build_instrument_defs(
    sf_instruments: List[sf2.SfInstrument], variants: _VariantBuilder
) -> List[InstrumentDef]:
    instruments: List[InstrumentDef] = []
    for sf_inst in sf_instruments:
        instrument = InstrumentDef(name=sf_inst.name)
        for region in sf_inst.regions:
            sample_index = variants.get_or_create(
                region.sample_id,
                region.root_note,
                region.fine_tune_cents,
                region.loop_start_offset,
                region.loop_end_offset,
                region.sample_modes,
            )
            if sample_index is None:
                continue  # unsupported sample (bad rate/type); notes fall back to the dummy
            instrument.regions.append(ResolvedRegion(region.key_low, region.key_high, sample_index))
        instruments.append(instrument)
    return instruments


def _add_orphan_variants(sf2_data: sf2.Sf2Data, variants: _VariantBuilder) -> None:
    """Ensures every SF2 sample is represented in the output even if no
    preset/instrument region references it, unless --remove-unused is set.
    Uses each sample's own natural (un-overridden) loop points and pitch.
    """
    for shdr_index, shdr in enumerate(sf2_data.samples):
        variants.get_or_create(
            shdr_index,
            root_note=shdr.original_pitch,
            fine_tune_cents=shdr.pitch_correction,
            loop_start_offset=0,
            loop_end_offset=0,
            sample_modes=_DEFAULT_SAMPLE_MODES_NO_LOOP,
        )


def _resolve_row(preset, instruments: List[InstrumentDef]) -> List[int]:
    row = [_NO_SAMPLE_INDEX] * 128
    for note in range(128):
        for zone in preset.zones:
            if not (zone.key_low <= note <= zone.key_high):
                continue
            if not (0 <= zone.instrument_index < len(instruments)):
                continue
            instrument = instruments[zone.instrument_index]
            for region in instrument.regions:
                if region.key_low <= note <= region.key_high:
                    row[note] = region.sample_index
                    break
            else:
                continue
            break
    return row


def _build_preset_infos_and_lookup(
    sf_presets: List[sf2.PresetDef], instruments: List[InstrumentDef]
) -> Tuple[List[PresetInfoOut], List[List[int]]]:
    melodic_by_program: Dict[int, sf2.PresetDef] = {}
    percussion_presets: List[sf2.PresetDef] = []

    for preset in sf_presets:
        if preset.bank == 0 and 0 <= preset.preset < MELODIC_INSTRUMENT_COUNT:
            melodic_by_program.setdefault(preset.preset, preset)
        elif preset.bank == PERCUSSION_BANK:
            percussion_presets.append(preset)

    presets: List[PresetInfoOut] = []
    lookup: List[List[int]] = []

    for program in range(MELODIC_INSTRUMENT_COUNT):
        preset = melodic_by_program.get(program)
        if preset is not None:
            presets.append(PresetInfoOut(preset.name, program, 0, program))
            lookup.append(_resolve_row(preset, instruments))
        else:
            presets.append(PresetInfoOut("", program, 0, program))
            lookup.append([_NO_SAMPLE_INDEX] * 128)

    for offset, preset in enumerate(percussion_presets):
        instrument_index = MELODIC_INSTRUMENT_COUNT + offset
        presets.append(PresetInfoOut(preset.name, instrument_index, PERCUSSION_BANK, preset.preset))
        lookup.append(_resolve_row(preset, instruments))

    return presets, lookup


def _build_pcm_blob_and_entries(samples: List[RawSample]) -> Tuple[bytes, List[SampleEntryOut]]:
    pcm_parts: List[bytes] = []
    entries: List[SampleEntryOut] = []
    offset = 0
    for sample in samples:
        entries.append(
            SampleEntryOut(
                pcm_offset=offset,
                length=sample.frame_count,
                loop_start=sample.loop_start,
                loop_end=sample.loop_end,
                root_note=sample.root_note,
                fine_tune=sample.fine_tune,
                flags=int(sample.loop_mode),
            )
        )
        pcm_parts.append(sample.pcm)
        offset += sample.frame_count
    return b"".join(pcm_parts), entries


def _remap_lookup(lookup: List[List[int]], remap: Dict[int, int]) -> List[List[int]]:
    # _NO_SAMPLE_INDEX is a sentinel, not a real (pre-dedup) sample index, so
    # it must pass through unchanged rather than being looked up in remap.
    return [[index if index == _NO_SAMPLE_INDEX else remap[index] for index in row] for row in lookup]


def _empty_bank() -> GMBankData:
    """A minimal, valid, always-compilable bank: a single dummy silent
    sample, one all-dummy lookup row and no presets. Used when the input
    file is missing or unparseable."""
    dummy = _dummy_sample()
    pcm, entries = _build_pcm_blob_and_entries([dummy])
    return GMBankData(sample_entries=entries, pcm_data=pcm, lookup=[[0] * 128], presets=[], instrument_count=0)


def convert(sf2_bytes: bytes, args: argparse.Namespace, warnings: Warnings, log: Logger) -> GMBankData:
    sf2_data = sf2.parse_sf2(sf2_bytes)
    log.log(f"parsed {len(sf2_data.presets)} preset(s), {len(sf2_data.instruments)} instrument(s), {len(sf2_data.samples)} sample(s)")

    sf_instruments = sf2.build_instruments(sf2_data, warnings, log)
    sf_presets = sf2.build_presets(sf2_data, warnings, log)

    channels = _ChannelAudioCache(sf2_data, warnings)
    variants = _VariantBuilder(sf2_data, channels, args.trim_silence, warnings)

    instruments = _build_instrument_defs(sf_instruments, variants)

    if not args.remove_unused:
        _add_orphan_variants(sf2_data, variants)

    presets, lookup = _build_preset_infos_and_lookup(sf_presets, instruments)

    unique_samples, remap = optimize.deduplicate(variants.instances)
    lookup = _remap_lookup(lookup, remap)
    log.log(f"{len(variants.instances)} sample instance(s) deduplicated to {len(unique_samples)}")

    pcm_data, sample_entries = _build_pcm_blob_and_entries(unique_samples)

    if args.max_size is not None and len(pcm_data) > args.max_size:
        warnings.add(f"PCM data is {len(pcm_data)} bytes, exceeding --max-size={args.max_size} bytes")

    instrument_count = MELODIC_INSTRUMENT_COUNT + max(0, len(lookup) - MELODIC_INSTRUMENT_COUNT)

    return GMBankData(
        sample_entries=sample_entries,
        pcm_data=pcm_data,
        lookup=lookup,
        presets=presets,
        instrument_count=instrument_count,
    )


def main(argv: List[str]) -> int:
    args = _parse_args(argv)
    warnings = Warnings()
    log = Logger(args.verbose)

    if not args.input.exists():
        warnings.add(f"input file '{args.input}' does not exist; generating an empty GMBank_data.generated.h")
        bank = _empty_bank()
    else:
        try:
            sf2_bytes = args.input.read_bytes()
            bank = convert(sf2_bytes, args, warnings, log)
        except ValueError as error:
            warnings.add(f"failed to parse '{args.input}': {error}; generating an empty GMBank_data.generated.h")
            bank = _empty_bank()

    output_path = emit_cpp.write_header(bank, args.output_directory)
    log.log(f"wrote {output_path}")
    warnings.emit()
    return 0
