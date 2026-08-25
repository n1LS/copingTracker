"""SoundFont 2 (SF2) chunk parsing and preset/instrument resolution.

This is the only module that knows about SoundFont concepts. Everything it
produces (:class:`~ctsb_converter.model.InstrumentDef`,
:class:`~ctsb_converter.model.PresetDef`, :class:`SampleHeader`) is plain
data; no SF2 vocabulary leaks past this module into audio processing or C++
generation.
"""

from __future__ import annotations

import enum
import struct
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

from .model import PresetDef, PresetZone
from .riff import RiffChunk, parse_riff
from .util import Logger, Warnings, clamp, sanitize_name

# --------------------------------------------------------------------------
# Raw chunk record shapes
# --------------------------------------------------------------------------

_PHDR_FORMAT = "<20sHHHIII"
_PHDR_SIZE = struct.calcsize(_PHDR_FORMAT)
_BAG_FORMAT = "<HH"
_BAG_SIZE = struct.calcsize(_BAG_FORMAT)
_GEN_FORMAT = "<H2s"
_GEN_SIZE = struct.calcsize(_GEN_FORMAT)
_INST_FORMAT = "<20sH"
_INST_SIZE = struct.calcsize(_INST_FORMAT)
_SHDR_FORMAT = "<20sIIIIIBbHH"
_SHDR_SIZE = struct.calcsize(_SHDR_FORMAT)


class Generator(enum.IntEnum):
    """SF2 generator operator IDs that this converter understands (values
    match the SoundFont 2.01 spec, section 8.1.2)."""

    START_ADDRS_OFFSET = 0
    END_ADDRS_OFFSET = 1
    STARTLOOP_ADDRS_OFFSET = 2
    ENDLOOP_ADDRS_OFFSET = 3
    START_ADDRS_COARSE_OFFSET = 4
    END_ADDRS_COARSE_OFFSET = 12
    PAN = 17
    DELAY_VOL_ENV = 33
    ATTACK_VOL_ENV = 34
    HOLD_VOL_ENV = 35
    DECAY_VOL_ENV = 36
    SUSTAIN_VOL_ENV = 37
    RELEASE_VOL_ENV = 38
    STARTLOOP_ADDRS_COARSE_OFFSET = 45
    KEY_RANGE = 43
    VEL_RANGE = 44
    ENDLOOP_ADDRS_COARSE_OFFSET = 50
    COARSE_TUNE = 51
    FINE_TUNE = 52
    SAMPLE_ID = 53
    SAMPLE_MODES = 54
    SCALE_TUNING = 56
    EXCLUSIVE_CLASS = 57
    OVERRIDING_ROOT_KEY = 58
    INSTRUMENT = 41
    END_OPER = 60


# Human-readable labels for SF2 generators that are not in the Generator enum
# (i.e. generators that are intentionally ignored). Used in warnings.
_GENERATOR_LABELS: dict[int, str] = {
    5: "modLfoToPitch",
    6: "vibLfoToPitch",
    7: "modEnvToPitch",
    8: "initialFilterFc (cutoff)",
    9: "initialFilterQ (resonance)",
    10: "modLfoToFilterFc",
    11: "modEnvToFilterFc",
    13: "modLfoToVolume",
    14: "unused",
    15: "chorusEffectsSend",
    16: "reverbEffectsSend",
    18: "unused",
    19: "unused",
    20: "unused",
    21: "delayModLFO",
    22: "freqModLFO",
    23: "delayVibLFO",
    24: "freqVibLFO",
    25: "delayModEnv",
    26: "attackModEnv",
    27: "holdModEnv",
    28: "decayModEnv",
    29: "sustainModEnv",
    30: "releaseModEnv",
    31: "keynumToModEnvHold",
    32: "keynumToModEnvDecay",
    39: "keynumToVolEnvHold",
    40: "keynumToVolEnvDecay",
    46: "keynum",
    47: "velocity",
    48: "initialAttenuation",
    49: "unused",
    55: "unused",
    59: "unused",
}


# Generators that are read explicitly and never reported as "ignored".
_HANDLED_GENERATORS = {
    Generator.START_ADDRS_OFFSET,
    Generator.END_ADDRS_OFFSET,
    Generator.STARTLOOP_ADDRS_OFFSET,
    Generator.ENDLOOP_ADDRS_OFFSET,
    Generator.START_ADDRS_COARSE_OFFSET,
    Generator.END_ADDRS_COARSE_OFFSET,
    Generator.STARTLOOP_ADDRS_COARSE_OFFSET,
    Generator.ENDLOOP_ADDRS_COARSE_OFFSET,
    Generator.KEY_RANGE,
    Generator.COARSE_TUNE,
    Generator.FINE_TUNE,
    Generator.SAMPLE_ID,
    Generator.SAMPLE_MODES,
    Generator.OVERRIDING_ROOT_KEY,
    Generator.INSTRUMENT,
    Generator.ATTACK_VOL_ENV,
    Generator.DECAY_VOL_ENV,
    Generator.SUSTAIN_VOL_ENV,
    Generator.RELEASE_VOL_ENV,
}
# Generators that are intentionally ignored (per spec) but do not, on their
# own, warrant a per-region "may affect playback" warning. DELAY_VOL_ENV and
# HOLD_VOL_ENV are read implicitly (they are folded away: the converted ADSR
# has no delay/hold stage, per the firmware's envelope design), so they are
# silently dropped rather than warned about.
_SILENTLY_IGNORED_GENERATORS = {
    Generator.VEL_RANGE,
    Generator.END_OPER,
    Generator.DELAY_VOL_ENV,
    Generator.HOLD_VOL_ENV,
}

_SAMPLE_MODE_LOOP_VALUES = {1, 3}  # 1 = loop continuously, 3 = loop then play to end


@dataclass
class PresetHeader:
    name: str
    preset: int
    bank: int
    bag_index: int


@dataclass
class Bag:
    gen_index: int
    mod_index: int


@dataclass
class SampleHeader:
    name: str
    start: int
    end: int
    loop_start: int
    loop_end: int
    sample_rate: int
    original_pitch: int
    pitch_correction: int
    sample_link: int
    sample_type: int


@dataclass
class InstrumentHeader:
    name: str
    bag_index: int


@dataclass
class Sf2Data:
    presets: List[PresetHeader]
    preset_bags: List[Bag]
    preset_gens: List[Tuple[int, bytes]]
    instruments: List[InstrumentHeader]
    inst_bags: List[Bag]
    inst_gens: List[Tuple[int, bytes]]
    samples: List[SampleHeader]
    pcm: bytes


# --------------------------------------------------------------------------
# Binary parsing
# --------------------------------------------------------------------------


def _iter_records(data: bytes, fmt: str, size: int):
    count = len(data) // size
    for record in struct.iter_unpack(fmt, data[: count * size]):
        yield record


def _parse_phdr(data: bytes) -> List[PresetHeader]:
    records = []
    for name, preset, bank, bag_index, _library, _genre, _morphology in _iter_records(
        data, _PHDR_FORMAT, _PHDR_SIZE
    ):
        records.append(PresetHeader(sanitize_name(name), preset, bank, bag_index))
    # Last record is the terminal "EOP" sentinel; drop it.
    return records[:-1] if records else records


def _parse_bags(data: bytes) -> List[Bag]:
    return [Bag(gen_index, mod_index) for gen_index, mod_index in _iter_records(data, _BAG_FORMAT, _BAG_SIZE)][:-1]


def _parse_gens(data: bytes) -> List[Tuple[int, bytes]]:
    return [(oper, raw) for oper, raw in _iter_records(data, _GEN_FORMAT, _GEN_SIZE)][:-1]


def _parse_inst(data: bytes) -> List[InstrumentHeader]:
    records = [InstrumentHeader(sanitize_name(name), bag_index) for name, bag_index in _iter_records(data, _INST_FORMAT, _INST_SIZE)]
    return records[:-1] if records else records


def _parse_shdr(data: bytes) -> List[SampleHeader]:
    records = []
    for (
        name,
        start,
        end,
        loop_start,
        loop_end,
        sample_rate,
        original_pitch,
        pitch_correction,
        sample_link,
        sample_type,
    ) in _iter_records(data, _SHDR_FORMAT, _SHDR_SIZE):
        records.append(
            SampleHeader(
                sanitize_name(name),
                start,
                end,
                loop_start,
                loop_end,
                sample_rate,
                original_pitch,
                pitch_correction,
                sample_link,
                sample_type,
            )
        )
    return records[:-1] if records else records


def parse_sf2(data: bytes) -> Sf2Data:
    """Parse the raw SF2 file bytes into structured (but still SF2-shaped)
    records. Raises ``ValueError`` if required chunks are missing.
    """
    root = parse_riff(data)
    if root.form_type != "sfbk":
        raise ValueError("not a SoundFont 2 file (missing 'sfbk' RIFF form)")

    pdta = root.find_list("pdta")
    sdta = root.find_list("sdta")
    if pdta is None:
        raise ValueError("SF2 file is missing the 'pdta' chunk")

    def chunk_bytes(container: Optional[RiffChunk], chunk_id: str) -> bytes:
        if container is None:
            return b""
        found = container.find(chunk_id)
        return found.data if found is not None else b""

    smpl = chunk_bytes(sdta, "smpl")

    return Sf2Data(
        presets=_parse_phdr(chunk_bytes(pdta, "phdr")),
        preset_bags=_parse_bags(chunk_bytes(pdta, "pbag")),
        preset_gens=_parse_gens(chunk_bytes(pdta, "pgen")),
        instruments=_parse_inst(chunk_bytes(pdta, "inst")),
        inst_bags=_parse_bags(chunk_bytes(pdta, "ibag")),
        inst_gens=_parse_gens(chunk_bytes(pdta, "igen")),
        samples=_parse_shdr(chunk_bytes(pdta, "shdr")),
        pcm=smpl,
    )


# --------------------------------------------------------------------------
# Generator zone helpers
# --------------------------------------------------------------------------


def _amount_short(raw: bytes) -> int:
    return struct.unpack("<h", raw)[0]


def _amount_range(raw: bytes) -> Tuple[int, int]:
    return raw[0], raw[1]


def _zone_generator_dicts(
    bags: List[Bag], gens: List[Tuple[int, bytes]]
) -> List[Dict[int, bytes]]:
    """Split the flat generator list into one dict per zone using the bag's
    gen_index ranges. The bag list already excludes its terminal sentinel.
    """
    zones: List[Dict[int, bytes]] = []
    for i, bag in enumerate(bags):
        start = bag.gen_index
        end = bags[i + 1].gen_index if i + 1 < len(bags) else len(gens)
        zone_gens: Dict[int, bytes] = {}
        for oper, raw in gens[start:end]:
            zone_gens[oper] = raw
        zones.append(zone_gens)
    return zones


def _is_global_zone(zone_gens: Dict[int, bytes], terminal_generator: Generator) -> bool:
    return terminal_generator not in zone_gens


def _warn_ignored_generators(
    zone_gens: Dict[int, bytes], context: str, warnings: Warnings, already_warned: set
) -> None:
    ignored = {
        oper
        for oper in zone_gens
        if oper not in _HANDLED_GENERATORS and oper not in _SILENTLY_IGNORED_GENERATORS
    }
    if not ignored:
        return
    key = (context, tuple(sorted(ignored)))
    if key in already_warned:
        return
    already_warned.add(key)
    def gen_label(oper: int) -> str:
        # Prefer the descriptive label from the lookup table (which includes
        # extra context like "(cutoff)" for filters), then fall back to the
        # Generator enum name, then to a raw number.
        label = _GENERATOR_LABELS.get(oper)
        if label is not None:
            return label
        try:
            return Generator(oper).name
        except ValueError:
            return f"gen#{oper}"

    names = ", ".join(gen_label(o) for o in sorted(ignored))
    # warnings.add(f"{context}: ignoring generator(s) that may affect playback: {names}")
    if Generator.VEL_RANGE in zone_gens:
        low, high = _amount_range(zone_gens[Generator.VEL_RANGE])
        if (low, high) != (0, 127):
            warnings.add(f"{context}: velocity layering collapsed (velRange {low}-{high} ignored)")


# --------------------------------------------------------------------------
# Instrument resolution
# --------------------------------------------------------------------------


@dataclass
class RawRegion:
    """An instrument zone, resolved down to the fields we care about, still
    expressed in *pre audio-processing* (original sample rate) terms.
    """

    key_low: int
    key_high: int
    sample_id: int
    root_note: int
    fine_tune_cents: int
    loop_start_offset: int
    loop_end_offset: int
    sample_modes: int
    attack: int
    decay: int
    sustain: int
    release: int


@dataclass
class SfInstrument:
    """An SF2 instrument with its zones resolved to :class:`RawRegion`
    (still referencing raw ``shdr`` sample indices, pre audio-processing).
    """

    name: str
    regions: List[RawRegion]


def _combine_pitch(
    shdr: SampleHeader, coarse_tune: int, fine_tune: int, overriding_root_key: Optional[int]
) -> Tuple[int, int]:
    """Combine SF2 pitch generators + sample header pitch fields into a
    (root_note, fine_tune_cents) pair, folding whole semitones from coarse
    tune/cents overflow into the root note (mirrors how the runtime derives
    pitch from rootNote + fineTune).
    """
    root_note = overriding_root_key if overriding_root_key is not None else shdr.original_pitch
    total_cents = coarse_tune * 100 + fine_tune + shdr.pitch_correction

    # Round to nearest semitone: if the fractional cents >= 50, round up to
    # the next semitone and store the negative remainder (e.g., 75 cents ->
    # 1 semitone + (-25) cents = "almost a semitone higher").
    semitone_shift = round(total_cents / 100.0)
    remainder = total_cents - semitone_shift * 100

    # A positive coarse/fine tune raises the perceived pitch, which is
    # equivalent to *lowering* the effective root note for a given played
    # note (see SampleInstrument::Start: offset = note - rootNote).
    root_note -= semitone_shift
    return root_note, remainder


# SF2 defaults (spec section 8.1.3) for the vol-env generators when absent
# from a zone: -12000 timecents is ~1ms (as close to "instant" as the
# timecents encoding gets), 0 centibels of attenuation is full volume.
_DEFAULT_ENV_TIMECENTS = -12000
_DEFAULT_SUSTAIN_CENTIBELS = 0

# Firmware-side envelope stage times are stored as milliseconds in a
# uint16_t, so anything at/above this is clamped (~65.5 seconds).
_MAX_ENV_MS = 0xFFFF


def _timecents_to_ms(timecents: int) -> int:
    """SF2 timecents -> milliseconds: time = 2^(timecents/1200) seconds."""
    seconds = 2.0 ** (timecents / 1200.0)
    return clamp(round(seconds * 1000.0), 0, _MAX_ENV_MS)


def _centibels_to_level(centibels: int) -> int:
    """SF2 sustain-vol-env centibels (0 = no attenuation/full volume, up to
    1000 cB = full attenuation/silence) -> a linear uint16_t amplitude level
    (0 = silent, 0xFFFF = full volume), matching how the firmware's envelope
    scales output samples.
    """
    centibels = clamp(centibels, 0, 1000)
    amplitude = 10.0 ** (-centibels / 200.0)
    return clamp(round(amplitude * _MAX_ENV_MS), 0, _MAX_ENV_MS)


def _resolve_envelope(merged: Dict[int, bytes]) -> Tuple[int, int, int, int]:
    """Resolve the DAHDSR vol-env generators (delay/hold intentionally
    dropped, per the firmware's plain-ADSR envelope) into (attack, decay,
    sustain, release), each scaled to a uint8_t. Attack/decay/release map to
    LUT indices via interpolation in the firmware; sustain maps from 0-65535
    to 0-255 (255 = full volume)."""
    attack_tc = (
        _amount_short(merged[Generator.ATTACK_VOL_ENV]) if Generator.ATTACK_VOL_ENV in merged else _DEFAULT_ENV_TIMECENTS
    )
    decay_tc = (
        _amount_short(merged[Generator.DECAY_VOL_ENV]) if Generator.DECAY_VOL_ENV in merged else _DEFAULT_ENV_TIMECENTS
    )
    release_tc = (
        _amount_short(merged[Generator.RELEASE_VOL_ENV]) if Generator.RELEASE_VOL_ENV in merged else _DEFAULT_ENV_TIMECENTS
    )
    sustain_cb = (
        _amount_short(merged[Generator.SUSTAIN_VOL_ENV]) if Generator.SUSTAIN_VOL_ENV in merged else _DEFAULT_SUSTAIN_CENTIBELS
    )

    attack_ms = _timecents_to_ms(attack_tc)
    decay_ms = _timecents_to_ms(decay_tc)
    release_ms = _timecents_to_ms(release_tc)
    sustain_level = _centibels_to_level(sustain_cb)

    # Quantize to uint8_t: sustain is divided by 256 to map from 0-65535 to 0-255.
    # For attack/decay/release, we map the millisecond values to 0-255 range
    # where 0 represents the fastest and 255 the slowest. The exact interpretation
    # depends on the firmware's LUT, but a simple linear scaling works:
    # We use 255 * (1 - e^(-t/tau)) where tau=256ms gives reasonable behavior.
    attack_u8 = clamp(round(attack_ms / 256.0 * 255), 0, 255)
    decay_u8 = clamp(round(decay_ms / 256.0 * 255), 0, 255)
    release_u8 = clamp(round(release_ms / 256.0 * 255), 0, 255)
    sustain_u8 = clamp(sustain_level >> 8, 0, 255)

    return attack_u8, decay_u8, sustain_u8, release_u8


def build_instruments(sf2: Sf2Data, warnings: Warnings, log: Logger) -> List[SfInstrument]:
    zones_per_instrument = _zone_generator_dicts(sf2.inst_bags, sf2.inst_gens)
    already_warned: set = set()
    instruments: List[SfInstrument] = []


    for inst_index, inst in enumerate(sf2.instruments):
        start = inst.bag_index
        end = sf2.instruments[inst_index + 1].bag_index if inst_index + 1 < len(sf2.instruments) else len(zones_per_instrument)
        zone_range = zones_per_instrument[start:end]

        global_gens: Dict[int, bytes] = {}
        local_zones = zone_range
        if zone_range and _is_global_zone(zone_range[0], Generator.SAMPLE_ID):
            global_gens = zone_range[0]
            local_zones = zone_range[1:]

        instrument = SfInstrument(name=inst.name, regions=[])
        for zone_gens in local_zones:
            if Generator.SAMPLE_ID not in zone_gens:
                continue  # malformed zone with no sample reference; skip

            merged = dict(global_gens)
            merged.update(zone_gens)
            _warn_ignored_generators(merged, f"instrument '{inst.name}'", warnings, already_warned)

            sample_id = _amount_short(merged[Generator.SAMPLE_ID])
            if sample_id < 0 or sample_id >= len(sf2.samples):
                warnings.add(f"instrument '{inst.name}': sample index {sample_id} out of range, skipping region")
                continue

            key_low, key_high = (0, 127)
            if Generator.KEY_RANGE in merged:
                key_low, key_high = _amount_range(merged[Generator.KEY_RANGE])

            coarse_tune = _amount_short(merged[Generator.COARSE_TUNE]) if Generator.COARSE_TUNE in merged else 0
            fine_tune = _amount_short(merged[Generator.FINE_TUNE]) if Generator.FINE_TUNE in merged else 0
            overriding_root_key = None
            if Generator.OVERRIDING_ROOT_KEY in merged:
                value = _amount_short(merged[Generator.OVERRIDING_ROOT_KEY])
                if value >= 0:
                    overriding_root_key = value

            loop_start_offset = 0
            loop_end_offset = 0
            if Generator.STARTLOOP_ADDRS_OFFSET in merged:
                loop_start_offset += _amount_short(merged[Generator.STARTLOOP_ADDRS_OFFSET])
            if Generator.STARTLOOP_ADDRS_COARSE_OFFSET in merged:
                loop_start_offset += _amount_short(merged[Generator.STARTLOOP_ADDRS_COARSE_OFFSET]) * 32768
            if Generator.ENDLOOP_ADDRS_OFFSET in merged:
                loop_end_offset += _amount_short(merged[Generator.ENDLOOP_ADDRS_OFFSET])
            if Generator.ENDLOOP_ADDRS_COARSE_OFFSET in merged:
                loop_end_offset += _amount_short(merged[Generator.ENDLOOP_ADDRS_COARSE_OFFSET]) * 32768

            sample_modes = _amount_short(merged[Generator.SAMPLE_MODES]) if Generator.SAMPLE_MODES in merged else 0

            shdr = sf2.samples[sample_id]
            root_note, fine_tune_cents = _combine_pitch(shdr, coarse_tune, fine_tune, overriding_root_key)
            attack, decay, sustain, release = _resolve_envelope(merged)

            region = RawRegion(
                key_low=key_low,
                key_high=key_high,
                sample_id=sample_id,
                root_note=root_note,
                fine_tune_cents=fine_tune_cents,
                loop_start_offset=loop_start_offset,
                loop_end_offset=loop_end_offset,
                sample_modes=sample_modes,
                attack=attack,
                decay=decay,
                sustain=sustain,
                release=release,
            )
            instrument.regions.append(region)

        instruments.append(instrument)
        log.log(f"instrument '{inst.name}': {len(instrument.regions)} region(s)")

    return instruments


def build_presets(sf2: Sf2Data, warnings: Warnings, log: Logger) -> List[PresetDef]:
    zones_per_preset = _zone_generator_dicts(sf2.preset_bags, sf2.preset_gens)
    already_warned: set = set()
    presets: List[PresetDef] = []

    for preset_index, phdr in enumerate(sf2.presets):
        start = phdr.bag_index
        end = sf2.presets[preset_index + 1].bag_index if preset_index + 1 < len(sf2.presets) else len(zones_per_preset)
        zone_range = zones_per_preset[start:end]

        global_gens: Dict[int, bytes] = {}
        local_zones = zone_range
        if zone_range and _is_global_zone(zone_range[0], Generator.INSTRUMENT):
            global_gens = zone_range[0]
            local_zones = zone_range[1:]

        preset = PresetDef(name=phdr.name, bank=phdr.bank, preset=phdr.preset)
        for zone_gens in local_zones:
            if Generator.INSTRUMENT not in zone_gens:
                continue

            merged = dict(global_gens)
            merged.update(zone_gens)
            _warn_ignored_generators(merged, f"preset '{phdr.name}'", warnings, already_warned)

            instrument_index = _amount_short(merged[Generator.INSTRUMENT])
            if instrument_index < 0:
                continue

            key_low, key_high = (0, 127)
            if Generator.KEY_RANGE in merged:
                key_low, key_high = _amount_range(merged[Generator.KEY_RANGE])

            preset.zones.append(PresetZone(key_low, key_high, instrument_index))

        presets.append(preset)
        log.log(f"preset '{phdr.name}' (bank {phdr.bank}, preset {phdr.preset}): {len(preset.zones)} zone(s)")

    return presets
