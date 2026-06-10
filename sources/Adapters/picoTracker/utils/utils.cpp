/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "utils.h"
#include "System/System/System.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include <System/Console/Trace.h>
#include <stdio.h>

void measure_freqs(void) {
  uint32_t f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
  uint32_t f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
  uint32_t f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
  uint32_t f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
  uint32_t f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
  uint32_t f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
#if PICO_RP2040
  uint32_t f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
#endif

  Trace::Debug("pll_sys  = %dkHz", f_pll_sys);
  Trace::Debug("pll_usb  = %dkHz", f_pll_usb);
  Trace::Debug("rosc     = %dkHz", f_rosc);
  Trace::Debug("clk_sys  = %dkHz", f_clk_sys);
  Trace::Debug("clk_peri = %dkHz", f_clk_peri);
  Trace::Debug("clk_usb  = %dkHz", f_clk_usb);
  Trace::Debug("clk_adc  = %dkHz", f_clk_adc);
#if PICO_RP2040
  Trace::Debug("clk_rtc  = %dkHz", f_clk_rtc);
#endif

  // Can't measure clk_ref / xosc as it is the ref
}
