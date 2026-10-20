/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "BasslineGenerator.h"
#include "System/Console/nanoprintf.h"
#include <algorithm>
#include <cmath>

namespace BasslineGenerator {

namespace {

constexpr uint8_t kMaxScale = 7;

struct ScaleDef {
  uint8_t size;
  uint8_t semis[kMaxScale];
};

const ScaleDef kScales[] = {
    {7, {0, 2, 3, 5, 7, 8, 10}}, // minor
    {7, {0, 2, 4, 5, 7, 9, 11}}, // major
    {5, {0, 3, 5, 7, 10}},       // minor pentatonic
    {5, {0, 2, 4, 7, 9}},        // major pentatonic
    {7, {0, 2, 3, 5, 7, 9, 10}}, // dorian
    {6, {0, 3, 5, 6, 7, 10}},    // minor blues
};

const char *const kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char *const kRoman[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};

// Rhythm templates, 16th note positions. Octave and pulse use plain eighths and
// let density add or remove events.
struct Template {
  uint8_t count;
  uint8_t steps[8];
};

const Template kFunky[] = {
    {8, {0, 3, 4, 7, 8, 10, 12, 15}}, {8, {0, 2, 4, 6, 8, 11, 12, 14}}, {7, {0, 3, 5, 8, 10, 12, 15}},
    {7, {0, 4, 6, 8, 11, 12, 14}},    {7, {0, 2, 5, 7, 8, 12, 14}},     {7, {0, 3, 4, 8, 10, 12, 14}},
};
const Template kEighths = {8, {0, 2, 4, 6, 8, 10, 12, 14}};

const uint8_t kFunkDegrees[kSteps] = {0, 0, 4, 0, 3, 0, 4, 6, 0, 4, 0, 3, 4, 0, 6, 4};

struct Rng {
  uint32_t s;
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  float f() { // [0, 1)
    return (next() >> 8) / 16777216.0f;
  }
  uint8_t below(uint8_t n) {
    return static_cast<uint8_t>(next() % n);
  }
};

inline int clampi(int v, int lo, int hi) {
  return std::max(lo, std::min(hi, v));
}

struct Chord {
  uint8_t rootPc;
  uint8_t tones[3]; // pitch classes
  bool hasTone(uint8_t pc) const {
    return tones[0] == pc || tones[1] == pc || tones[2] == pc;
  }
};

bool inScale(const Settings &s, uint8_t pc) {
  const ScaleDef &sc = kScales[static_cast<uint8_t>(s.scale)];
  for (uint8_t i = 0; i < sc.size; ++i) {
    if ((s.key + sc.semis[i]) % 12 == pc) {
      return true;
    }
  }
  return false;
}

Chord chordFor(const Settings &s, uint8_t degree) {
  const ScaleDef &sc = kScales[static_cast<uint8_t>(s.scale)];
  const uint8_t n = sc.size;
  Chord c;
  c.rootPc = (s.key + sc.semis[degree % n]) % 12;
  c.tones[0] = c.rootPc;
  c.tones[1] = (s.key + sc.semis[(degree + 2) % n]) % 12;
  c.tones[2] = (s.key + sc.semis[(degree + 4) % n]) % 12;
  return c;
}

// Rhythm of the first phrase. Templates keep their shape, density adds or
// removes events (quarter notes are the last to go, step 0 always stays).
uint16_t makeMask(const Settings &s, Rng &rng) {
  const Template &t = s.style == Style::Funky ? kFunky[rng.below(sizeof(kFunky) / sizeof(kFunky[0]))] : kEighths;
  uint16_t mask = 0;
  for (uint8_t i = 0; i < t.count; ++i) {
    mask |= 1u << t.steps[i];
  }

  const float density = std::pow(s.density / 255.0f, 0.8f);
  const int target = clampi(static_cast<int>(std::lround(2 + density * 12)), 2, 14);

  auto popcount = [](uint16_t m) {
    int c = 0;
    for (; m; m &= m - 1) {
      ++c;
    }
    return c;
  };

  while (popcount(mask) > target) {
    uint8_t off[kSteps], quarter[kSteps];
    uint8_t offN = 0, quarterN = 0;
    for (uint8_t i = 1; i < kSteps; ++i) {
      if (mask & (1u << i)) {
        (i % 4 ? off : quarter)[i % 4 ? offN++ : quarterN++] = i;
      }
    }
    if (offN) {
      mask &= ~(1u << off[rng.below(offN)]);
    } else if (quarterN) {
      mask &= ~(1u << quarter[rng.below(quarterN)]);
    } else {
      break;
    }
  }

  while (popcount(mask) < target) {
    uint8_t pool[2 * kSteps];
    uint8_t n = 0;
    for (uint8_t i = 0; i < kSteps; ++i) {
      if (mask & (1u << i)) {
        continue;
      }
      if (s.style == Style::Funky && i % 4 == 0) {
        continue; // funk keeps the downbeats for the motif
      }
      pool[n++] = i;
      if (i % 4 == 2) {
        pool[n++] = i; // offbeat eighths are twice as likely
      }
    }
    if (!n) {
      break;
    }
    mask |= 1u << pool[rng.below(n)];
  }

  return mask | 1u;
}

uint8_t chooseDegree(const Settings &s, Rng &rng, uint8_t step) {
  const uint8_t n = kScales[static_cast<uint8_t>(s.scale)].size;
  const float movement = s.movement / 255.0f;
  if (step == 0) {
    return 0;
  }
  switch (s.style) {
    case Style::Pulse:
      return rng.f() < 0.82f ? 0 : (rng.f() < movement ? 4 % n : 0);
    case Style::Octave:
      return (step % 2 == 1 && rng.f() < movement * 0.8f) ? rng.below(n) : 0;
    default:
      {
        if (rng.f() < movement * 0.25f) {
          const uint8_t choices[4] = {static_cast<uint8_t>(kFunkDegrees[step] % n), 0, static_cast<uint8_t>(4 % n),
                                      static_cast<uint8_t>(3 % n)};
          return choices[rng.below(4)];
        }
        return kFunkDegrees[step] % n;
      }
  }
}

uint8_t pitchForDegree(const Settings &s, uint8_t degree, uint8_t step, Rng &rng, int prev) {
  const ScaleDef &sc = kScales[static_cast<uint8_t>(s.scale)];
  int midi = 36 + s.key + sc.semis[degree % sc.size];
  if (s.style == Style::Octave && (step / 2) % 2 == 1) {
    midi += 12;
  }
  const bool lift = rng.f() < 0.12f;
  if (lift && step != 0) {
    midi += 12;
  }
  if (prev >= 0 && std::abs(midi - prev) > 12 && rng.f() < 0.75f) {
    midi += midi > prev ? -12 : 12;
  }
  return static_cast<uint8_t>(clampi(midi, kLowestNote, kHighestNote));
}

// Moves the notes of a phrase onto the active chords. Notes already in the
// chord stay, passing tones (scale notes off the beat) stay as far as the
// movement allows, anything else snaps to the nearest chord tone.
void retune(uint8_t *phrase, const uint8_t *slots, const Settings &s) {
  const uint8_t n = kScales[static_cast<uint8_t>(s.scale)].size;
  const float movement = s.movement / 255.0f;

  uint8_t active[kChordSlots];
  uint8_t current = 0;
  for (uint8_t q = 0; q < kChordSlots; ++q) {
    if (slots[q] != kCarry) {
      current = std::min<uint8_t>(slots[q], n - 1);
    }
    active[q] = current;
  }

  int ordinal = 0;
  for (uint8_t i = 0; i < kSteps; ++i) {
    const int midi = phrase[i];
    if (midi == kNoNote) {
      continue;
    }
    const Chord chord = chordFor(s, active[i / 4]);
    const uint8_t pc = midi % 12;
    const float roll = ((i * 37 + midi * 13) % 100) / 100.0f;

    if (s.style == Style::Octave) {
      int low = 36 + chord.rootPc;
      while (low > 43) {
        low -= 12;
      }
      while (low < 28) {
        low += 12;
      }
      if (i % 2 == 0) {
        phrase[i] = low + (ordinal % 2 ? 12 : 0);
        ordinal++;
        continue;
      }
      if (!chord.hasTone(pc) && inScale(s, pc) && roll < movement * 0.95f) {
        continue;
      }
      phrase[i] = ordinal ? low + ((ordinal - 1) % 2 ? 12 : 0) : low;
      continue;
    }

    const float boost = s.style == Style::Funky ? 1.05f : 0.25f;
    const float passing = std::min(0.95f, movement * boost);
    if (i % 4 != 0 && inScale(s, pc) && roll < passing) {
      continue;
    }
    if (chord.hasTone(pc)) {
      continue;
    }

    int best = midi, bestScore = 1 << 20;
    for (int m = kLowestNote; m <= 55; ++m) {
      if (!chord.hasTone(m % 12)) {
        continue;
      }
      const int score = std::abs(m - midi) + (i % 4 == 0 && m % 12 != chord.rootPc ? 2 : 0);
      if (score < bestScore) {
        bestScore = score;
        best = m;
      }
    }
    phrase[i] = best;
  }
}

} // namespace

uint8_t scaleSize(Scale scale) {
  return kScales[static_cast<uint8_t>(scale)].size;
}

void chordLabel(uint8_t key, Scale scale, uint8_t degree, char *out, uint8_t size) {
  const ScaleDef &sc = kScales[static_cast<uint8_t>(scale)];
  const uint8_t n = sc.size;
  degree %= n;
  const int root = sc.semis[degree];
  const int third = (sc.semis[(degree + 2) % n] - root + 12) % 12;
  const int fifth = (sc.semis[(degree + 4) % n] - root + 12) % 12;

  char roman[6];
  const char *r = kRoman[degree % 7];
  uint8_t len = 0;
  for (; r[len] && len < 3; ++len) {
    roman[len] = third == 3 ? (r[len] | 0x20) : r[len]; // minor chords in lower case
  }
  if (fifth == 6) {
    roman[len++] = 'o';
  } else if (third == 3 && fifth == 10) {
    roman[len++] = '7';
  }
  roman[len] = 0;

  npf_snprintf(out, size, "%s %s", roman, kNoteNames[(key + root) % 12]);
}

bool generate(const Settings &s, Chain &out, uint32_t seed) {
  out.phraseCount = 0;
  if (s.phraseCount < 1 || s.phraseCount > kMaxPhrases || s.key > 11 || s.style >= Style::Count ||
      s.scale >= Scale::Count) {
    return false;
  }

  const uint8_t n = scaleSize(s.scale);
  Rng rng{seed ? seed : 1};

  // --- first phrase: rhythm, then pitches ---
  const uint16_t mask = makeMask(s, rng);
  uint8_t *first = out.note[0];
  int prev = -1;
  for (uint8_t step = 0; step < kSteps; ++step) {
    if (!(mask & (1u << step))) {
      first[step] = kNoNote;
      continue;
    }
    uint8_t degree = chooseDegree(s, rng, step);
    const float accent = s.style == Style::Funky ? 0.36f : s.style == Style::Pulse ? 0.12f : 0.72f;
    if (s.style != Style::Octave && step % 4 == 0 && rng.f() < accent) {
      const uint8_t anchors[4] = {0, 0, 0, static_cast<uint8_t>(4 % n)};
      degree = anchors[rng.below(4)];
    }
    first[step] = pitchForDegree(s, degree, step, rng, prev);
    prev = first[step];
  }

  // --- later phrases: controlled mutations of the first ---
  const float v = s.variation / 255.0f;
  const float amountScale = std::pow(v, 0.72f);
  for (uint8_t p = 1; p < s.phraseCount; ++p) {
    uint8_t *next = out.note[p];
    std::copy(first, first + kSteps, next);
    Rng mut{(seed ? seed : 1) ^ (0x9E3779B9u * (p + 1))};
    if (!mut.s) {
      mut.s = 1;
    }

    // pitch changes, step 0 stays the tonal anchor
    uint8_t eligible[kSteps];
    uint8_t count = 0;
    for (uint8_t i = 1; i < kSteps; ++i) {
      if (next[i] != kNoNote) {
        eligible[count++] = i;
      }
    }
    const int amount = static_cast<int>(std::lround(amountScale * (p == 1 ? 4 : p == 2 ? 7 : 10)));
    for (int m = 0; m < amount && count; ++m) {
      const uint8_t k = mut.below(count);
      const uint8_t at = eligible[k];
      eligible[k] = eligible[--count];
      next[at] = pitchForDegree(s, mut.below(n), at, mut, next[at]);
    }

    // rhythm changes on off-beat steps
    uint8_t cand[kSteps];
    count = 0;
    for (uint8_t i = 1; i < kSteps; ++i) {
      if (i % 4 != 0) {
        cand[count++] = i;
      }
    }
    const int rhythm = static_cast<int>(std::lround(v * (p == 1 ? 2 : p == 2 ? 4 : 6)));
    for (int m = 0; m < rhythm && count; ++m) {
      const uint8_t k = mut.below(count);
      const uint8_t at = cand[k];
      cand[k] = cand[--count];
      if (next[at] == kNoNote) {
        next[at] = pitchForDegree(s, chooseDegree(s, mut, at), at, mut, -1);
      } else {
        next[at] = kNoNote;
      }
    }

    // the last phrase may add a pickup into the loop
    if (p == s.phraseCount - 1 && v > 0.2f && mut.f() < v && next[14] == kNoNote) {
      next[14] = pitchForDegree(s, 0, 14, mut, -1);
    }
    next[0] = first[0];
  }

  // --- harmony ---
  for (uint8_t p = 0; p < s.phraseCount; ++p) {
    uint8_t slots[kChordSlots];
    std::copy(s.chords[p], s.chords[p] + kChordSlots, slots);
    if (p == 0 && slots[0] == kCarry) {
      slots[0] = 0;
    }
    retune(out.note[p], slots, s);
  }

  out.phraseCount = s.phraseCount;
  return true;
}

} // namespace BasslineGenerator
