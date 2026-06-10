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

#define INSTRUMENT_FILE_EXTENSION       ".ins"
#define THEME_FILE_EXTENSION            ".thm"

#define UNNAMED_PROJECT_NAME            ".untitled"
#define CONFIG_FILENAME                 ".config.xml"
#define PROJECT_FILENAME                "project.xml"
#define AUTO_SAVE_FILENAME              "autosave.xml"

#define SD_BASE_DIR         "/copingTracker"

#define PROJECTS_DIR        SD_BASE_DIR "/projects"
#define SAMPLES_LIB_DIR     SD_BASE_DIR "/samples"
#define INSTRUMENTS_DIR     SD_BASE_DIR "/instruments"
#define RENDERS_DIR         SD_BASE_DIR "/renders"
#define THEMES_DIR          SD_BASE_DIR "/themes"
#define RECORDINGS_DIR      SD_BASE_DIR "/recordings"
#define CONFIG_FILE_PATH    SD_BASE_DIR "/" CONFIG_FILENAME

#define PROJECT_SAMPLES_DIR             "samples"
#define RECORDING_FILENAME "REC01.wav"

#endif // _PERSISTENCE_CONSTANTS_H_
