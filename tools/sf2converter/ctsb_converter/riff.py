"""Minimal RIFF container parser.

This module knows nothing about SoundFont 2 semantics: it only understands
the generic RIFF chunk format (``RIFF``/``LIST`` containers plus leaf
chunks). SF2-specific chunk interpretation lives in :mod:`sf2`.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import List, Optional

_CONTAINER_IDS = ("RIFF", "LIST")


@dataclass
class RiffChunk:
    chunk_id: str
    form_type: Optional[str]
    data: bytes
    children: List["RiffChunk"] = field(default_factory=list)

    def find(self, chunk_id: str) -> Optional["RiffChunk"]:
        for child in self.children:
            if child.chunk_id == chunk_id:
                return child
        return None

    def find_list(self, form_type: str) -> Optional["RiffChunk"]:
        for child in self.children:
            if child.chunk_id == "LIST" and child.form_type == form_type:
                return child
        return None


def _read_chunk(buf: bytes, offset: int) -> "tuple[RiffChunk, int]":
    if offset + 8 > len(buf):
        raise ValueError("truncated RIFF chunk header")

    chunk_id = buf[offset:offset + 4].decode("ascii", errors="replace")
    size = struct.unpack_from("<I", buf, offset + 4)[0]
    body_start = offset + 8
    body_end = body_start + size
    if body_end > len(buf):
        raise ValueError(f"truncated RIFF chunk '{chunk_id}'")
    # RIFF chunks are word-aligned; a size of odd length is padded by one byte.
    next_offset = body_end + (size & 1)

    if chunk_id in _CONTAINER_IDS:
        form_type = buf[body_start:body_start + 4].decode("ascii", errors="replace")
        children: List[RiffChunk] = []
        pos = body_start + 4
        while pos < body_end:
            child, pos = _read_chunk(buf, pos)
            children.append(child)
        return RiffChunk(chunk_id, form_type, b"", children), next_offset

    return RiffChunk(chunk_id, None, buf[body_start:body_end], []), next_offset


def parse_riff(data: bytes) -> RiffChunk:
    """Parse a complete RIFF file and return its root chunk."""
    if len(data) < 12 or data[0:4] != b"RIFF":
        raise ValueError("not a RIFF file")
    chunk, _ = _read_chunk(data, 0)
    return chunk
