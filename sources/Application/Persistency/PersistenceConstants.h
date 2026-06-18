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
#define MAX_THEME_EXPORT_PATH_LENGTH (MAX_THEME_NAME_LENGTH + strlen(THEMES_DIR) + 1 + strlen(THEME_FILE_EXTENSION))
// accounts for .ins extension so they are 4 chars shorter.
#define MAX_INSTRUMENT_NAME_LENGTH (MAX_INSTRUMENT_FILENAME_LENGTH - 4)

#define INSTRUMENT_FILE_EXTENSION ".ins"
#define THEME_FILE_EXTENSION ".thm"

#define UNNAMED_PROJECT_NAME ".untitled"
#define CONFIG_FILENAME ".config.xml"
#define PROJECT_FILENAME "project.xml"
#define AUTO_SAVE_FILENAME "autosave.xml"

#define SD_BASE_DIR "/copingTracker"

#define PROJECTS_DIR SD_BASE_DIR "/projects"
#define SAMPLES_LIB_DIR SD_BASE_DIR "/samples"
#define INSTRUMENTS_DIR SD_BASE_DIR "/instruments"
#define RENDERS_DIR SD_BASE_DIR "/renders"
#define THEMES_DIR SD_BASE_DIR "/themes"
#define CONFIG_FILE_PATH SD_BASE_DIR "/" CONFIG_FILENAME

#define PROJECT_SAMPLES_DIR "samples"

/* xml attributes and keys ***************************************************/

#define XML_ATTR_VERSION "version"
#define XML_ATTR_VALUE "value"
#define XML_ATTR_NAME "name"
#define XML_ATTR_TYPE "type"
#define XML_ATTR_ID "id"
#define XML_ATTR_TABLE_RATIO "table-ratio"
#define XML_ATTR_LENGTH "length"
#define XML_ATTR_SLICE_PREFIX "sl"
#define XML_ATTR_TABLE_ID "table-id"
#define XML_ATTR_ID "id"
#define XML_ATTR_INSTRUMENT_NAME "instrument-name"

#define XML_ELEM_PARAMETER "parameter"
#define XML_ELEM_INSTRUMENT "instrument"
#define XML_ELEM_TABLE "table"
#define XML_ELEM_COLOR "color"
#define XML_ELEM_SONG "song"
#define XML_ELEM_THEME "theme"
#define XML_ELEM_BASE "coping-tracker"
#define XML_ELEM_DATA "data"
#define XML_ELEM_CONFIGURATION "configuration"
#define XML_ELEM_FONT "font"
#define XML_ELEM_THEME_NAME "theme-name"
#define XML_ELEM_COMMAND1 "command1"
#define XML_ELEM_COMMAND2 "command2"
#define XML_ELEM_COMMAND3 "command3"
#define XML_ELEM_VALUE1 "value1"
#define XML_ELEM_VALUE2 "value2"
#define XML_ELEM_VALUE3 "value3"
#define XML_ELEM_CHAIN_STEPS "chain-steps"
#define XML_ELEM_PHRASE_STEPS "phrase-steps"

#endif // _PERSISTENCE_CONSTANTS_H_
