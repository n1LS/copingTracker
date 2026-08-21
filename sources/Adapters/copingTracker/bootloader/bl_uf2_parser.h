/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_UF2_PARSER_H
#define PATCHBAY_BL_UF2_PARSER_H

#include "Externals/SdFat/src/SdFat.h"

int flash_derived_bin_to_slot(SdFs *sd, const char *bin_path);
int convert_uf2_to_bin(SdFs *sd, const char *filename, uint32_t target_slot,
                       const char *derived_output_path);

#endif // PATCHBAY_BL_UF2_PARSER_H