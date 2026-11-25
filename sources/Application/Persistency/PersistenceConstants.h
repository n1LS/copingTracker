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

#ifndef _PERSISTENCE_CONSTANTS_H_
#define _PERSISTENCE_CONSTANTS_H_

#define MAX_PROJECT_NAME_LENGTH 16
// Sample filenames include the ".wav" extension.
#define MAX_INSTRUMENT_FILENAME_LENGTH 24
#define MAX_THEME_NAME_LENGTH 16
// MAX_THEME_NAME_LENGTH (16) + "/copingTracker/themes" (20) + "." (1) + "thm" (3) = 40 bytes
#define MAX_THEME_EXPORT_PATH_LENGTH 64
// Theme name + extension: "themename" (16) + ".thm" (4) = 20 bytes
#define MAX_THEME_FILENAME_LENGTH 32
// accounts for .ins extension so they are 4 chars shorter.
#define MAX_INSTRUMENT_NAME_LENGTH 16

#define INSTRUMENT_FILE_EXTENSION ".ins"
#define THEME_FILE_EXTENSION ".thm"

#define UNNAMED_PROJECT_NAME ".untitled"
#define CONFIG_FILENAME ".config.json"
#define PROJECT_FILENAME "project.json"
#define AUTO_SAVE_FILENAME "autosave.json"

#define SD_BASE_DIR "/copingTracker"

#define PROJECTS_DIR SD_BASE_DIR "/projects"
#define SAMPLES_LIB_DIR SD_BASE_DIR "/samples"
#define INSTRUMENTS_DIR SD_BASE_DIR "/instruments"
#define RENDERS_DIR SD_BASE_DIR "/renders"
#define THEMES_DIR SD_BASE_DIR "/themes"
#define CONFIG_FILE_PATH SD_BASE_DIR "/" CONFIG_FILENAME

#define PROJECT_SAMPLES_DIR "samples"

/* json keys: kebab-case ****************************************************/

// members of the project file, one per Persistent
#define JSON_NODE_PROJECT "project"
#define JSON_NODE_SONG "song"
#define JSON_NODE_TABLES "tables"
#define JSON_NODE_GROOVES "grooves"
#define JSON_NODE_INSTRUMENT_BANK "instrument-bank"
#define JSON_NODE_MIXER "mixer"
// the root of an instrument file is the instrument itself, this is only the name its Persistent registers under
#define JSON_NODE_INSTRUMENT "instrument"

// keys inside of them
#define JSON_KEY_VERSION "version"
#define JSON_KEY_TYPE "type"
#define JSON_KEY_NAME "name"
#define JSON_KEY_ID "id"
#define JSON_KEY_STEPS "steps"
#define JSON_KEY_VARIABLES "variables"
#define JSON_KEY_EFFECTS "effects"
#define JSON_KEY_SLICES "slices"
#define JSON_KEY_COLORS "colors"
#define JSON_KEY_ROWS "rows"
#define JSON_KEY_CHAINS "chains"
#define JSON_KEY_PHRASES "phrases"
#define JSON_KEY_TABLES "tables"
#define JSON_KEY_GROOVES "grooves"
#define JSON_KEY_INSTRUMENTS "instruments"
#define JSON_KEY_TABLE_RATIO "table-ratio"

#endif // _PERSISTENCE_CONSTANTS_H_
