/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "DrumPhraseGenerator.h"

namespace DrumGenerator {

static constexpr int kDrumTemplateCount = 48;

static constexpr DrumTemplate kDrumTemplates[kDrumTemplateCount] = {
  // ============================================================
  // STRAIGHT — 16 steps
  // ============================================================

  // Four-on-the-floor, conventional backbeat, open hats.
  {Style::Straight, Feel::Straight, 16,
    {0x1111, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x8080, 0x0001}},

  // Pop groove with a full sixteenth-note hat line.
  {Style::Straight, Feel::Straight, 16,
    {0x0501, 0x0000, 0x1010, 0x1010, 0x0000, 0x0000,
     0xFFFF, 0x0000, 0x4000, 0x0000, 0x8080, 0x0000}},

  // Straight groove with an extra kick pickup.
  {Style::Straight, Feel::Straight, 16,
    {0x1119, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x4444, 0x0000}},

  // Driving pop-rock groove with snare and clap reinforcement.
  {Style::Straight, Feel::Straight, 16,
    {0x1111, 0x0000, 0x1010, 0x1010, 0x0000, 0x0000,
     0x7777, 0x0000, 0x0000, 0x0000, 0x8888, 0x0001}},

  // ============================================================
  // SPARSE — 16 steps
  // ============================================================

  // Restrained kick, solid backbeat, eighth-note hats.
  {Style::Sparse, Feel::Straight, 16,
    {0x0401, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x4000, 0x0001}},

  // Minimal groove with a tom pickup at the end.
  {Style::Sparse, Feel::Straight, 16,
    {0x0101, 0x0000, 0x1010, 0x0000, 0x0000, 0x8000,
     0x4444, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000}},

  // Spacious groove with a late kick and occasional open hat.
  {Style::Sparse, Feel::Straight, 16,
    {0x1001, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x1111, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000}},

  // Sparse, slightly lopsided groove with a rim accent.
  {Style::Sparse, Feel::Straight, 16,
    {0x0401, 0x0800, 0x1010, 0x0000, 0x0000, 0x0000,
     0x4444, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001}},

  // ============================================================
  // SYNCOPATED — 16 steps
  // ============================================================

  // Offbeat kicks against a stable backbeat.
  {Style::Syncopated, Feel::Straight, 16,
    {0x0489, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0808, 0x0000, 0x8080, 0x0000}},

  // Irregular kick placement, with a clap accent.
  {Style::Syncopated, Feel::Straight, 16,
    {0x4241, 0x0000, 0x1010, 0x1000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x2020, 0x0000}},

  // Busy syncopated kick with sparse hats.
  {Style::Syncopated, Feel::Straight, 16,
    {0x2489, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x4545, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000}},

  // Syncopated snare accents and open-hat punctuation.
  {Style::Syncopated, Feel::Straight, 16,
    {0x0881, 0x0000, 0x1410, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x4040, 0x0001}},

  // ============================================================
  // BROKEN — 16 steps
  // ============================================================

  // Displaced snare, rim accents, and a tom pickup.
  {Style::Broken, Feel::Straight, 16,
    {0x0481, 0x8080, 0x0808, 0x0000, 0x0000, 0x8000,
     0x4D4D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},

  // Uneven kick/snare interplay with scattered hats.
  {Style::Broken, Feel::Straight, 16,
    {0x4221, 0x0000, 0x1008, 0x0000, 0x1000, 0x0000,
     0x6565, 0x0000, 0x8080, 0x0000, 0x0000, 0x0000}},

  // Broken beat with a displaced snare and tom punctuation.
  {Style::Broken, Feel::Straight, 16,
    {0x0881, 0x4040, 0x0810, 0x0000, 0x0000, 0x4000,
     0x4949, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001}},

  // Sparse, jagged rhythm with late accents.
  {Style::Broken, Feel::Straight, 16,
    {0x2141, 0x0000, 0x1020, 0x0000, 0x0000, 0x0800,
     0x2525, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000}},

  // ============================================================
  // DRIVING — 16 steps
  // ============================================================

  // Rock-like pulse, open hats on the offbeats.
  {Style::Driving, Feel::Straight, 16,
    {0x1111, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x4444, 0x0101}},

  // Busy hats and a more active kick.
  {Style::Driving, Feel::Straight, 16,
    {0x1191, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0xFFFF, 0x0000, 0x0000, 0x0000, 0x4040, 0x0001}},

  // Driving groove with a firm four-beat kick.
  {Style::Driving, Feel::Straight, 16,
    {0x1111, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0x7777, 0x0000, 0x0000, 0x0000, 0x8888, 0x1000}},

  // Dense kick and hat pattern with a small snare variation.
  {Style::Driving, Feel::Straight, 16,
    {0x1511, 0x0000, 0x1010, 0x0000, 0x0000, 0x0000,
     0xFFFF, 0x0000, 0x0000, 0x0000, 0x2222, 0x0001}},

  // ============================================================
  // HALF-TIME — 16 steps
  // ============================================================

  // Strong snare on step 8, with a sparse kick.
  {Style::HalfTime, Feel::Straight, 16,
    {0x0401, 0x0000, 0x0100, 0x0000, 0x0000, 0x8000,
     0x5555, 0x0000, 0x0000, 0x0000, 0x4000, 0x0001}},

  // Half-time backbeat with extra clap and tom accents.
  {Style::HalfTime, Feel::Straight, 16,
    {0x0481, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000,
     0x5555, 0x8000, 0x0000, 0x0000, 0x4040, 0x0000}},

  // Deep half-time groove with a late kick pickup.
  {Style::HalfTime, Feel::Straight, 16,
    {0x0501, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000,
     0x1111, 0x0000, 0x0000, 0x0000, 0x4000, 0x0001}},

  // Half-time groove with rim accents and a restrained hat line.
  {Style::HalfTime, Feel::Straight, 16,
    {0x0401, 0x8080, 0x0100, 0x0000, 0x0000, 0x0000,
     0x4545, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},

  // ============================================================
  // STRAIGHT — 8 steps
  // ============================================================

  // Simple eighth-note groove.
  {Style::Straight, Feel::Straight, 8,
    {0x0011, 0x0000, 0x0044, 0x0000, 0x0000, 0x0000,
     0x00FF, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000}},

  // Strong pulse with clap reinforcement.
  {Style::Straight, Feel::Straight, 8,
    {0x0055, 0x0000, 0x0044, 0x0044, 0x0000, 0x0000,
     0x0055, 0x0000, 0x0000, 0x0000, 0x0088, 0x0001}},

  // ============================================================
  // SPARSE — 8 steps
  // ============================================================

  // Kick on the downbeat, restrained hats.
  {Style::Sparse, Feel::Straight, 8,
    {0x0001, 0x0000, 0x0044, 0x0000, 0x0000, 0x0000,
     0x0055, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000}},

  // Spacious groove with a late kick.
  {Style::Sparse, Feel::Straight, 8,
    {0x0011, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000,
     0x0011, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000}},

  // ============================================================
  // SYNCOPATED — 8 steps
  // ============================================================

  // Extra kicks between the main beats.
  {Style::Syncopated, Feel::Straight, 8,
    {0x0029, 0x0000, 0x0044, 0x0000, 0x0000, 0x0000,
     0x005D, 0x0000, 0x0020, 0x0000, 0x0080, 0x0000}},

  // Offbeat snare and kick interplay.
  {Style::Syncopated, Feel::Straight, 8,
    {0x0049, 0x0000, 0x0054, 0x0000, 0x0000, 0x0000,
     0x00DD, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000}},

  // ============================================================
  // BROKEN — 8 steps
  // ============================================================

  // Displaced snare, rim accent, tom pickup.
  {Style::Broken, Feel::Straight, 8,
    {0x0021, 0x0040, 0x0084, 0x0000, 0x0000, 0x0080,
     0x005D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},

  // Sparse, jagged beat with a late snare.
  {Style::Broken, Feel::Straight, 8,
    {0x0041, 0x0080, 0x0014, 0x0000, 0x0000, 0x0000,
     0x0049, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001}},

  // ============================================================
  // DRIVING — 8 steps
  // ============================================================

  // Kick on every quarter-note beat.
  {Style::Driving, Feel::Straight, 8,
    {0x0055, 0x0000, 0x0044, 0x0000, 0x0000, 0x0000,
     0x00FF, 0x0000, 0x0000, 0x0000, 0x0088, 0x0001}},

  // Driving pulse with clap and open-hat accents.
  {Style::Driving, Feel::Straight, 8,
    {0x0055, 0x0000, 0x0044, 0x0044, 0x0000, 0x0000,
     0x0055, 0x0000, 0x0000, 0x0000, 0x00AA, 0x0001}},

  // ============================================================
  // HALF-TIME — 8 steps
  // ============================================================

  // Snare on step 4.
  {Style::HalfTime, Feel::Straight, 8,
    {0x0021, 0x0000, 0x0010, 0x0000, 0x0000, 0x0080,
     0x0055, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000}},

  // Deep kick, half-time snare, and occasional clap.
  {Style::HalfTime, Feel::Straight, 8,
    {0x0009, 0x0000, 0x0010, 0x0010, 0x0000, 0x0000,
     0x0011, 0x0000, 0x0000, 0x0000, 0x0080, 0x0000}},

  // ============================================================
  // STRAIGHT — 12-step triplet feel
  // ============================================================

  // Triplet hats with a conventional backbeat.
  {Style::Straight, Feel::Triplet, 12,
    {0x0041, 0x0000, 0x0208, 0x0000, 0x0000, 0x0000,
     0x0B6D, 0x0000, 0x0000, 0x0000, 0x0800, 0x0001}},

  // Steady triplet pulse with clap reinforcement.
  {Style::Straight, Feel::Triplet, 12,
    {0x0249, 0x0000, 0x0208, 0x0208, 0x0000, 0x0000,
     0x0FFF, 0x0000, 0x0000, 0x0000, 0x0924, 0x0001}},

  // ============================================================
  // SPARSE — 12-step triplet feel
  // ============================================================

  // Quarter-note hats and a straightforward backbeat.
  {Style::Sparse, Feel::Triplet, 12,
    {0x0041, 0x0000, 0x0208, 0x0000, 0x0000, 0x0000,
     0x0249, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000}},

  // Very open triplet groove with occasional tom.
  {Style::Sparse, Feel::Triplet, 12,
    {0x0001, 0x0000, 0x0200, 0x0000, 0x0000, 0x0400,
     0x0049, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001}},

  // ============================================================
  // SYNCOPATED — 12-step triplet feel
  // ============================================================

  // Kicks off the main pulse, extra hat taps.
  {Style::Syncopated, Feel::Triplet, 12,
    {0x0121, 0x0000, 0x0208, 0x0000, 0x0000, 0x0000,
     0x0B6D, 0x0000, 0x0820, 0x0000, 0x0100, 0x0000}},

  // Syncopated kick and snare with open-hat accents.
  {Style::Syncopated, Feel::Triplet, 12,
    {0x0149, 0x0000, 0x0220, 0x0000, 0x0000, 0x0000,
     0x0DB6, 0x0000, 0x0000, 0x0000, 0x0400, 0x0001}},

  // ============================================================
  // BROKEN — 12-step triplet feel
  // ============================================================

  // Displaced snare, rim accent, and tom pickup.
  {Style::Broken, Feel::Triplet, 12,
    {0x0111, 0x0040, 0x0204, 0x0000, 0x0000, 0x0800,
     0x0555, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},

  // Broken triplet phrase with a late snare and tom.
  {Style::Broken, Feel::Triplet, 12,
    {0x0089, 0x0200, 0x0104, 0x0000, 0x0000, 0x0400,
     0x0492, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001}},

  // ============================================================
  // DRIVING — 12-step triplet feel
  // ============================================================

  // Persistent triplet hats and regular kicks.
  {Style::Driving, Feel::Triplet, 12,
    {0x0249, 0x0000, 0x0208, 0x0000, 0x0000, 0x0000,
     0x0FFF, 0x0000, 0x0000, 0x0000, 0x0924, 0x0041}},

  // Full triplet pulse with extra open hats.
  {Style::Driving, Feel::Triplet, 12,
    {0x0249, 0x0000, 0x0208, 0x0000, 0x0000, 0x0000,
     0x0FFF, 0x0000, 0x0000, 0x0000, 0x0AAA, 0x0001}},

  // ============================================================
  // HALF-TIME — 12-step triplet feel
  // ============================================================

  // Widely spaced kick and snare.
  {Style::HalfTime, Feel::Triplet, 12,
    {0x0101, 0x0000, 0x0040, 0x0000, 0x0000, 0x0400,
     0x0B6D, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000}},

  // Sparse triplet hats with a heavy half-time backbeat.
  {Style::HalfTime, Feel::Triplet, 12,
    {0x0041, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000,
     0x0249, 0x0000, 0x0000, 0x0000, 0x0800, 0x0001}},
};

} // namespace DrumGenerator