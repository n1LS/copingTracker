/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef __BASIC_TYPEDEFS__
#define __BASIC_TYPEDEFS__

#include <stdint.h>

/* ----------------------------------------------------------------------------
 */

// basic types:
/* COMMENTED FOR PSP

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef uint16_t uint16;
typedef signed int int32;
typedef unsigned int uint32;

*/

/* ----------------------------------------------------------------------------
 */

int16_t Swap16(int16_t from);
int Swap32(int from);

#endif // __BASIC_TYPEDEFS__
