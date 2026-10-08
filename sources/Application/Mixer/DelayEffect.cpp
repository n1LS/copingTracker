/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "DelayEffect.h"
#include <string.h>

DelayEffect::DelayEffect() : writePos_(0), delaySlots_(LineSize / 2), feedback_(0x60), wet_(FP_ONE / 2) {
  Clear();
}

void DelayEffect::Clear() {
  memset(line_, 0, sizeof(line_));
  memset(send_, 0, sizeof(send_));
  phase_ = 0;
  cur_ = 0;
  active_ = false;
  silentSlots_ = LineSize;
}

void DelayEffect::SetParameters(int delaySlots, int feedback, fixed wet) {
  if (delaySlots < 1) {
    delaySlots = 1;
  } else if (delaySlots > LineSize) {
    delaySlots = LineSize;
  }
  delaySlots_ = delaySlots;
  feedback_ = feedback;
  wet_ = wet;
}

void DelayEffect::AddSend(const fixed *buffer, int samplecount) {
  // mono and 2:1 average in one go: each slot sums 4 values (2 samples, L+R)
  for (int i = 0; i < samplecount; i++) {
    int32_t r = fp2i(buffer[2 * i]);
    int32_t l = fp2i(buffer[2 * i + 1]);
    send_[(phase_ + i) >> 1] += (l + r) >> 2;
  }
  active_ = true;
}

bool DelayEffect::Render(fixed *buffer, int samplecount) {
  // nothing fed in and the line has died out, no need to run
  if (!active_ && silentSlots_ >= LineSize) {
    send_[0] = 0;
    cur_ = 0;
    phase_ = (phase_ + samplecount) & 1;
    return false;
  }

  fixed *dst = buffer;
  for (int i = 0; i < samplecount; i++) {
    int pos = phase_ + i;
    int s;
    if (!(pos & 1)) {
      // first half of a slot: fetch the delayed sample and interpolate
      // halfway towards it
      int8_t d = line_[readPos()];
      s = (cur_ + d) >> 1;
      cur_ = d;
    } else {
      // second half: the slot's input is complete, write it to the line
      // together with the feedback. Feedback rounds towards zero so the tail
      // can't get stuck at -1
      int fb = cur_ * feedback_;
      fb = (fb + (fb < 0 ? 255 : 0)) >> 8;
      int w = (send_[pos >> 1] >> 8) + fb;
      if (w > 127) {
        w = 127;
      } else if (w < -128) {
        w = -128;
      }
      line_[writePos_] = (int8_t)w;
      writePos_ = (writePos_ + 1 == LineSize) ? 0 : writePos_ + 1;
      silentSlots_ = (w == 0) ? silentSlots_ + 1 : 0;
      s = cur_;
    }
    fixed v = fp_mul_coef(i2fp(s << 8), wet_);
    *dst++ = v;
    *dst++ = v;
  }

  // carry a half filled slot over to the next buffer, clear the rest
  int end = phase_ + samplecount;
  int32_t carry = (end & 1) ? send_[end >> 1] : 0;
  memset(send_, 0, sizeof(int32_t) * ((end >> 1) + 1));
  send_[0] = carry;
  phase_ = end & 1;
  active_ = false;

  return true;
}
