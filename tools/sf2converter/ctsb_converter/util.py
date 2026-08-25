"""Small, dependency-free helpers shared across the converter.

Kept deliberately free of global mutable state: callers own a ``Warnings``
and ``Logger`` instance and thread them through the pipeline explicitly.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass, field
from typing import List


@dataclass
class Warnings:
    """Collects human readable warnings produced while converting."""

    messages: List[str] = field(default_factory=list)

    def add(self, message: str) -> None:
        self.messages.append(message)

    def emit(self, stream=sys.stderr) -> None:
        for message in self.messages:
            print(f"warning: {message}", file=stream)


class Logger:
    """Minimal verbose-only logger. Avoids global state / logging config."""

    def __init__(self, verbose: bool) -> None:
        self._verbose = verbose

    def log(self, message: str) -> None:
        if self._verbose:
            print(message, file=sys.stderr)


def parse_size(value: str) -> int:
    """Parse a ``--max-size`` argument, accepting plain byte counts or
    ``k``/``m`` suffixes (case-insensitive), e.g. ``512k``, ``2m``, ``1048576``.
    """
    text = value.strip().lower()
    multiplier = 1
    if text.endswith("k"):
        multiplier = 1024
        text = text[:-1]
    elif text.endswith("m"):
        multiplier = 1024 * 1024
        text = text[:-1]
    return int(text) * multiplier


def sanitize_name(raw: bytes) -> str:
    """Decode a fixed-size, NUL-padded SF2 name field into a clean string."""
    text = raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")
    return text.strip()


def clamp(value: int, low: int, high: int) -> int:
    return max(low, min(high, value))
