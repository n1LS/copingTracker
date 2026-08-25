"""cTSB Converter.

Offline tool that converts a SoundFont 2 (.sf2) file into a generated C++
header (``GMBank_data.generated.h``) consumed by the copingTracker firmware.

The firmware never parses SoundFont data or knows anything about SoundFont
concepts: this package performs all parsing, audio processing and lookup
table construction offline, and emits only immutable ``constexpr`` data.
"""

__version__ = "1.0.0"
