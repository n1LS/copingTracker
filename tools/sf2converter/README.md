# cTSB Converter

Offline Python 3 tool that converts a SoundFont 2 (`.sf2`) file into
`GMBank_data.generated.h`: a header of immutable `constexpr` data consumed by
the copingTracker firmware's (handwritten) `GMBank.h`/`GMBank.cpp`.

The firmware never parses SF2 data or knows anything about SoundFont
concepts — all of that happens here, offline.

## Usage

```sh
cd tools/sf2converter
python3 -m ctsb_converter input.sf2 output_directory
```

Options:

```
--mono            accepted for compatibility; output is always mono
--trim-silence    trim leading/trailing silence (never into a loop region)
--remove-unused   drop samples not referenced by any preset/instrument
--verbose         print progress information to stderr
--max-size=<n>    warn if the generated PCM blob exceeds <n> bytes (accepts k/m suffixes)
```

If `input.sf2` does not exist (or fails to parse), the tool still writes a
valid, minimal `GMBank_data.generated.h` (a dummy silent sample, an all-dummy
lookup table, zero presets) so the firmware continues to compile.

## Layout

| Module         | Responsibility                                          |
|----------------|----------------------------------------------------------|
| `__main__.py`  | `python -m ctsb_converter` entry point                    |
| `cli.py`       | Argument parsing and the top-level conversion pipeline     |
| `riff.py`      | Generic RIFF container parsing (no SF2 knowledge)          |
| `sf2.py`       | SF2 chunk parsing + preset/instrument/region resolution    |
| `model.py`     | Shared, format-agnostic dataclasses                         |
| `audio.py`     | Mono downmix, 44.1kHz->22.05kHz decimation, silence trim    |
| `optimize.py`  | Deduplication of identical finalized samples                |
| `emit_cpp.py`  | Writes `GMBank_data.generated.h`                                |
| `util.py`      | Warning collection, logging, small helpers                  |

## Notes on fidelity

- Runtime PCM is always mono, 16-bit, 22050 Hz. 44100 Hz input is
  downsampled by simple 2:1 decimation; any other source rate is skipped
  with a warning.
- Only forward looping is supported (SF2 `sampleModes` 1/3); reserved or
  unrecognized modes are downgraded to one-shot with a warning.
- SF2 `coarseTune`/`fineTune`/`pitchCorrection` are combined and folded into
  whole semitones (root note) plus a signed remainder (`fineTune`, ~128
  units per semitone, matching the firmware's existing fine-tune scale).
- Only generators required for pitch, key range and looping are honored;
  velocity layers, filters, envelopes, LFOs, effects and exclusive classes
  are ignored (with warnings when they may affect playback).
- Output is deterministic: running the converter twice on the same input
  produces a byte-identical header.
