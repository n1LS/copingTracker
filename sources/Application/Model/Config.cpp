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

#include "Config.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Persistency/PersistencyDocument.h"
#include "Externals/etl/include/etl/flat_map.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/string_utilities.h"
#include "Foundation/Types/Types.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/Console/nanoprintf.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/I_File.h"
#include "ThemeConstants.h"
#include "Variable.h"
#include <stdlib.h>

#define CONFIG_VERSION_NUMBER 1

#define MIDI_DEVICE_LEN 4

#define CONFIG(X, A, B, D, E) {Token(X).c_str(), A, X, B, D, E}

static const char *lineOutOptions[3] = {"HP Low", "HP High", "Line Level"};
static const char *midiDeviceList[MIDI_DEVICE_LEN] = {"Off", "TRS", "USB", "TRS+USB"};
static const char *midiSendSync[2] = {"Off", "Send"};
static const char *midiClockSyncOptions[2] = {"Internal", "External"};
static const char *mirrorUIOnOff[2] = {"Off", "On"};
static const char *importResamplerOptions[] = {"None", "Linear"};
static const char *commandPickerOptions[] = {"Inline", "Picker"};
static constexpr int kImportResamplerOptionCount = 2;

// Param keys MUST fit in this length limit!
typedef etl::string<13> ParamString;

// Use default color values from ThemeConstants.h
// Other default values not related to theme colors:
constexpr int DEFAULT_LINEOUT = 0x2;
constexpr int DEFAULT_MIDIDEVICE = 0x0;
constexpr int DEFAULT_MIDISYNC = 0x0;
constexpr int DEFAULT_REMOTEUI = 0x1;
constexpr int DEFAULT_BACKLIGHT_LEVEL = 0xFF; // Default to max brightness (255)
constexpr int DEFAULT_OUTPUT_VOLUME = 40;
constexpr int DEFAULT_IMPORT_RESAMPLER = 0; // default for picoTracker is none (as original)
constexpr int DEFAULT_USE_COMMAND_PICKER = 1;

// Use a struct to define parameter information
struct ConfigParam {
  const char *name;
  union {
    int intValue;
    const char *strValue;
  } defaultValue;
  Token::enum_type fourcc;
  const char **options;
  int optionCount;
  bool isString;
};

// Define parameters as a static array instead of a ETL flat_map for example,
// because using a flat_map static requires too much stack space for
// initialization
static const ConfigParam configParams[] = {
    // Color variables
    CONFIG(Token::VarColor_0, {.intValue = ThemeConstants::DEFAULT_COLOR0}, nullptr, 0, false),
    CONFIG(Token::VarColor_1, {.intValue = ThemeConstants::DEFAULT_COLOR1}, nullptr, 0, false),
    CONFIG(Token::VarColor_2, {.intValue = ThemeConstants::DEFAULT_COLOR2}, nullptr, 0, false),
    CONFIG(Token::VarColor_3, {.intValue = ThemeConstants::DEFAULT_COLOR3}, nullptr, 0, false),
    CONFIG(Token::VarColor_4, {.intValue = ThemeConstants::DEFAULT_COLOR4}, nullptr, 0, false),
    CONFIG(Token::VarColor_5, {.intValue = ThemeConstants::DEFAULT_COLOR5}, nullptr, 0, false),
    CONFIG(Token::VarColor_6, {.intValue = ThemeConstants::DEFAULT_COLOR6}, nullptr, 0, false),
    CONFIG(Token::VarColor_7, {.intValue = ThemeConstants::DEFAULT_COLOR7}, nullptr, 0, false),
    CONFIG(Token::VarColor_8, {.intValue = ThemeConstants::DEFAULT_COLOR8}, nullptr, 0, false),
    CONFIG(Token::VarColor_9, {.intValue = ThemeConstants::DEFAULT_COLOR9}, nullptr, 0, false),
    CONFIG(Token::VarColor_A, {.intValue = ThemeConstants::DEFAULT_COLOR10}, nullptr, 0, false),
    CONFIG(Token::VarColor_B, {.intValue = ThemeConstants::DEFAULT_COLOR11}, nullptr, 0, false),
    CONFIG(Token::VarColor_C, {.intValue = ThemeConstants::DEFAULT_COLOR12}, nullptr, 0, false),
    CONFIG(Token::VarColor_D, {.intValue = ThemeConstants::DEFAULT_COLOR13}, nullptr, 0, false),
    CONFIG(Token::VarColor_E, {.intValue = ThemeConstants::DEFAULT_COLOR14}, nullptr, 0, false),
    CONFIG(Token::VarColor_F, {.intValue = ThemeConstants::DEFAULT_COLOR15}, nullptr, 0, false),

    CONFIG(Token::VarThemeName, {.strValue = ThemeConstants::DEFAULT_THEME_NAME}, nullptr, 0, true),

    // Device settings with options
    CONFIG(Token::VarLineOut, {.intValue = DEFAULT_LINEOUT}, lineOutOptions, 3, false),
    CONFIG(Token::VarMidiDevice, {.intValue = DEFAULT_MIDIDEVICE}, midiDeviceList, 4, false),
    CONFIG(Token::VarMidiSync, {.intValue = DEFAULT_MIDISYNC}, midiSendSync, 2, false),
    CONFIG(Token::VarMirrorUI, {.intValue = DEFAULT_REMOTEUI}, mirrorUIOnOff, 2, false),
    CONFIG(Token::VarUIFont, {.intValue = ThemeConstants::DEFAULT_UIFONT}, ThemeConstants::THEME_FONT_NAMES,
           ThemeConstants::THEME_FONT_COUNT, false),

    // Display brightness setting
    CONFIG(Token::VarBacklightLevel, {.intValue = DEFAULT_BACKLIGHT_LEVEL}, nullptr, 0, false),
    CONFIG(Token::VarOutputVolume, {.intValue = DEFAULT_OUTPUT_VOLUME}, nullptr, 0, false),
    CONFIG(Token::VarImportResampler, {.intValue = DEFAULT_IMPORT_RESAMPLER}, importResamplerOptions,
           kImportResamplerOptionCount, false),
    CONFIG(Token::VarConfigCommandPicker, {.intValue = DEFAULT_USE_COMMAND_PICKER}, commandPickerOptions, 2, false),
};

Config::Config()
    : VariableContainer(&variables_), color0_(Token::VarColor_0, static_cast<int>(ThemeConstants::DEFAULT_COLOR0)),
      color1_(Token::VarColor_1, static_cast<int>(ThemeConstants::DEFAULT_COLOR1)),
      color2_(Token::VarColor_2, static_cast<int>(ThemeConstants::DEFAULT_COLOR2)),
      color3_(Token::VarColor_3, static_cast<int>(ThemeConstants::DEFAULT_COLOR3)),
      color4_(Token::VarColor_4, static_cast<int>(ThemeConstants::DEFAULT_COLOR4)),
      color5_(Token::VarColor_5, static_cast<int>(ThemeConstants::DEFAULT_COLOR5)),
      color6_(Token::VarColor_6, static_cast<int>(ThemeConstants::DEFAULT_COLOR6)),
      color7_(Token::VarColor_7, static_cast<int>(ThemeConstants::DEFAULT_COLOR7)),
      color8_(Token::VarColor_8, static_cast<int>(ThemeConstants::DEFAULT_COLOR8)),
      color9_(Token::VarColor_9, static_cast<int>(ThemeConstants::DEFAULT_COLOR9)),
      color10_(Token::VarColor_A, static_cast<int>(ThemeConstants::DEFAULT_COLOR10)),
      color11_(Token::VarColor_B, static_cast<int>(ThemeConstants::DEFAULT_COLOR11)),
      color12_(Token::VarColor_C, static_cast<int>(ThemeConstants::DEFAULT_COLOR12)),
      color13_(Token::VarColor_D, static_cast<int>(ThemeConstants::DEFAULT_COLOR13)),
      color14_(Token::VarColor_E, static_cast<int>(ThemeConstants::DEFAULT_COLOR14)),
      color15_(Token::VarColor_F, static_cast<int>(ThemeConstants::DEFAULT_COLOR15)),

      lineOut_(Token::VarLineOut, lineOutOptions, 3, DEFAULT_LINEOUT),
      midiDevice_(Token::VarMidiDevice, midiDeviceList, 4, DEFAULT_MIDIDEVICE),
      midiSync_(Token::VarMidiSync, midiSendSync, 2, DEFAULT_MIDISYNC),
      mirrorUI_(Token::VarMirrorUI, mirrorUIOnOff, 2, DEFAULT_REMOTEUI),
      importResampler_(Token::VarImportResampler, importResamplerOptions, kImportResamplerOptionCount,
                       DEFAULT_IMPORT_RESAMPLER),
      commandInputMode_(Token::VarConfigCommandPicker, commandPickerOptions, 2, DEFAULT_USE_COMMAND_PICKER),
      uiFont_(Token::VarUIFont, ThemeConstants::THEME_FONT_NAMES, ThemeConstants::THEME_FONT_COUNT,
              ThemeConstants::DEFAULT_UIFONT),
      themeName_(Token::VarThemeName, ThemeConstants::DEFAULT_THEME_NAME),
      backlightLevel_(Token::VarBacklightLevel, DEFAULT_BACKLIGHT_LEVEL),
      outputVolume_(Token::VarOutputVolume, DEFAULT_OUTPUT_VOLUME) {

  variables_.push_back(&color0_);
  variables_.push_back(&color1_);
  variables_.push_back(&color2_);
  variables_.push_back(&color3_);
  variables_.push_back(&color4_);
  variables_.push_back(&color5_);
  variables_.push_back(&color6_);
  variables_.push_back(&color7_);
  variables_.push_back(&color8_);
  variables_.push_back(&color9_);
  variables_.push_back(&color10_);
  variables_.push_back(&color11_);
  variables_.push_back(&color12_);
  variables_.push_back(&color13_);
  variables_.push_back(&color14_);
  variables_.push_back(&color15_);
  variables_.push_back(&lineOut_);
  variables_.push_back(&midiDevice_);
  variables_.push_back(&midiSync_);
  variables_.push_back(&mirrorUI_);
  variables_.push_back(&importResampler_);
  variables_.push_back(&commandInputMode_);
  variables_.push_back(&uiFont_);
  variables_.push_back(&themeName_);
  variables_.push_back(&backlightLevel_);
  variables_.push_back(&outputVolume_);

  PersistencyDocument doc;

  if (!doc.Load(CONFIG_FILE_PATH)) {
    Trace::Error("CONFIG Could not open file for reading: %s", CONFIG_FILE_PATH);
    Save(); // and write the defaults to SDCard
    return;
  }

  bool elem = doc.FirstChild();
  if (!elem || strcmp(doc.ElemName(), XML_ELEM_CONFIGURATION)) {
    Trace::Log("CONFIG", "Bad " CONFIG_FILE_PATH " format!");
    // TODO: need show user some UI that config file is invalid
    Save(); // and write the defaults to SDCard
    return;
  }
  elem = doc.FirstChild(); // now get first child element of CONFIG
  while (elem) {
    // Check if the parameter exists in our parameters list
    bool paramFound = false;
    for (const auto &param : configParams) {
      if (strcmp(doc.ElemName(), param.name) == 0) {
        paramFound = true;
        break;
      }
    }

    // Special handling for Color elements
    if (strcmp(doc.ElemName(), XML_ELEM_COLOR) == 0) {
      // Process Color element
      ReadColorVariable(&doc);
      elem = doc.NextSibling();
      continue;
    }

    if (!paramFound) {
      Trace::Log("CONFIG", "Found unknown config parameter \"%s\", skipping...", doc.ElemName());
      elem = doc.NextSibling();
      continue;
    }
    bool hasAttr = doc.NextAttribute();
    while (hasAttr) {
      // Special handling for Theme Name sadly because it is a string and no
      // easy way to look that that up in configParams data above
      if (!strcmp(doc.ElemName(), XML_ELEM_THEME_NAME)) {
        if (Variable *themeVar = FindVariable(Token::VarThemeName)) {
          themeVar->SetString(doc.attrval_);
          Trace::Log("CONFIG", "Read Theme Name:%s", doc.attrval_);
        }
      } else {
        // Find the variable by name in configParams
        for (const auto &param : configParams) {
          if (!strcmp(doc.ElemName(), param.name)) {
            if (Variable *var = FindVariable(param.fourcc)) {
              var->SetInt(atoi(doc.attrval_));
              Trace::Log("CONFIG", "Set %s = %s", param.name, doc.attrval_);
            }
            break;
          }
        }
      }
      hasAttr = doc.NextAttribute();
    }
    elem = doc.NextSibling();
  }
  Trace::Log("CONFIG", "Loaded successfully");
}

Config::~Config() {
}

bool Config::Save() {
  auto fs = FileSystem::GetInstance();
  auto fp = fs->Open(CONFIG_FILE_PATH, "w");
  if (!fp) {
    Trace::Error("Could not open file for writing: %s", CONFIG_FILE_PATH);
    return false;
  }
  Trace::Log("PERSISTENCYSERVICE", "Opened Proj File: %s", CONFIG_FILE_PATH);
  tinyxml2::XMLPrinter printer(fp.get());

  SaveContent(&printer);

  return fp->Sync();
}

// Write color variables to an XMLPrinter using the same format as in
// SaveContent
void Config::WriteColorVariables(tinyxml2::XMLPrinter *printer) {
  auto it = variables_.begin();
  for (size_t i = 0; i < variables_.size(); i++) {
    Variable *var = *it;
    Token id = var->GetID();

    // Check if this is a color variable
    if (id == Token::VarColor_0 || id == Token::VarColor_1 || id == Token::VarColor_2 || id == Token::VarColor_3 ||
        id == Token::VarColor_4 || id == Token::VarColor_5 || id == Token::VarColor_6 || id == Token::VarColor_7 ||
        id == Token::VarColor_8 || id == Token::VarColor_9 || id == Token::VarColor_A || id == Token::VarColor_B ||
        id == Token::VarColor_C || id == Token::VarColor_D || id == Token::VarColor_E || id == Token::VarColor_F) {

      // Open a Color element
      printer->OpenElement(XML_ELEM_COLOR);

      // Add name attribute
      printer->PushAttribute(XML_ATTR_NAME, var->GetName());

      // Format color value in hex format with # prefix
      char hexValue[16];
      npf_snprintf(hexValue, sizeof(hexValue), "#%06X", var->GetInt());

      // Add value attribute in hex format
      printer->PushAttribute(XML_ATTR_VALUE, hexValue);

      // Close the Color element
      printer->CloseElement();
    }
    it++;
  }
}

void Config::ReadColorVariable(PersistencyDocument *doc) {
  // Process the current element if it's a Color element
  if (strcmp(doc->ElemName(), XML_ELEM_COLOR) == 0) {
    // Process Color element
    char colorName[64] = {0};
    char colorValue[64] = {0};

    // Get the name and value attributes
    while (doc->NextAttribute()) {
      if (strcmp(doc->attrname_, XML_ATTR_NAME) == 0) {
        // Use safer string copy to ensure null-termination
        size_t len = strlen(doc->attrval_);
        if (len >= sizeof(colorName)) {
          len = sizeof(colorName) - 1; // Truncate if too long
        }
        memcpy(colorName, doc->attrval_, len);
        colorName[len] = '\0'; // Ensure null-termination
      } else if (strcmp(doc->attrname_, XML_ATTR_VALUE) == 0) {
        // Use safer string copy to ensure null-termination
        size_t len = strlen(doc->attrval_);
        if (len >= sizeof(colorValue)) {
          len = sizeof(colorValue) - 1; // Truncate if too long
        }
        memcpy(colorValue, doc->attrval_, len);
        colorValue[len] = '\0'; // Ensure null-termination
      }
    }

    // If we have both name and value, set the variable
    if (colorName[0] != '\0' && colorValue[0] != '\0') {
      // Parse the color value (hex string)
      int value = 0;
      bool parsedSuccessfully = false;

      // Handle both formats: with # prefix and without
      if (colorValue[0] == '#' && sscanf(colorValue + 1, "%x", &value) == 1) {
        // Successfully parsed hex value with # prefix
        parsedSuccessfully = true;
      } else if (sscanf(colorValue, "%x", &value) == 1) {
        // Successfully parsed hex value without prefix
        parsedSuccessfully = true;
      } else {
        // Try decimal parsing for backward compatibility
        value = atoi(colorValue);
        if (value > 0) {
          parsedSuccessfully = true;
        }
      }

      if (parsedSuccessfully) {
        // Find the variable by name and set its value
        Token fourcc = Token::Default; // Use Default as invalid marker

        if (strcmp(colorName, Token(Token::VarColor_0).c_str()) == 0) {
          fourcc = Token::VarColor_0;
        } else if (strcmp(colorName, Token(Token::VarColor_1).c_str()) == 0) {
          fourcc = Token::VarColor_1;
        } else if (strcmp(colorName, Token(Token::VarColor_2).c_str()) == 0) {
          fourcc = Token::VarColor_2;
        } else if (strcmp(colorName, Token(Token::VarColor_3).c_str()) == 0) {
          fourcc = Token::VarColor_3;
        } else if (strcmp(colorName, Token(Token::VarColor_4).c_str()) == 0) {
          fourcc = Token::VarColor_4;
        } else if (strcmp(colorName, Token(Token::VarColor_5).c_str()) == 0) {
          fourcc = Token::VarColor_5;
        } else if (strcmp(colorName, Token(Token::VarColor_6).c_str()) == 0) {
          fourcc = Token::VarColor_6;
        } else if (strcmp(colorName, Token(Token::VarColor_7).c_str()) == 0) {
          fourcc = Token::VarColor_7;
        } else if (strcmp(colorName, Token(Token::VarColor_8).c_str()) == 0) {
          fourcc = Token::VarColor_8;
        } else if (strcmp(colorName, Token(Token::VarColor_9).c_str()) == 0) {
          fourcc = Token::VarColor_9;
        } else if (strcmp(colorName, Token(Token::VarColor_A).c_str()) == 0) {
          fourcc = Token::VarColor_A;
        } else if (strcmp(colorName, Token(Token::VarColor_B).c_str()) == 0) {
          fourcc = Token::VarColor_B;
        } else if (strcmp(colorName, Token(Token::VarColor_C).c_str()) == 0) {
          fourcc = Token::VarColor_C;
        } else if (strcmp(colorName, Token(Token::VarColor_D).c_str()) == 0) {
          fourcc = Token::VarColor_D;
        } else if (strcmp(colorName, Token(Token::VarColor_E).c_str()) == 0) {
          fourcc = Token::VarColor_E;
        } else if (strcmp(colorName, Token(Token::VarColor_F).c_str()) == 0) {
          fourcc = Token::VarColor_F;
        }

        if (fourcc != Token::Default) { // If we found a valid color
          Variable *var = FindVariable(fourcc);
          if (var) {
            var->SetInt(value);
            Trace::Log("CONFIG", "Read Color: %s = %d", colorName, value);
          }
        } else {
          Trace::Log("CONFIG", "Failed to read Color: %s = %d", colorName, value);
        }
      }
    }
  }
}

bool Config::SaveTheme(tinyxml2::XMLPrinter *printer, const char *themeName) {
  Trace::Log("CONFIG", "Saving theme content to XML");

  // Open the THEME root element
  printer->OpenElement(XML_ELEM_THEME);

  // We don't need to save the theme name in the file
  // The filename itself serves as the theme name

  // Save the font setting
  Variable *fontVar = FindVariable(Token::VarUIFont);
  if (fontVar) {
    printer->OpenElement(XML_ELEM_FONT);
    char buf[16];
    npf_snprintf(buf, sizeof(buf), "%d", fontVar->GetInt());
    printer->PushAttribute(XML_ATTR_VALUE, buf);
    printer->CloseElement(); // Font
  }

  // Write color variables
  WriteColorVariables(printer);

  // Close the THEME root element
  printer->CloseElement(); // THEME

  return true;
}

void Config::SaveContent(tinyxml2::XMLPrinter *printer) {
  // Log the number of variables in the list before saving
  Trace::Log("CONFIG", "Saving %d variables to config file", variables_.size());

  // store config version
  printer->OpenElement(XML_ELEM_CONFIGURATION);
  printer->PushAttribute(XML_ATTR_VERSION, CONFIG_VERSION_NUMBER);
  // save all of the config parameters
  auto it = variables_.begin();
  for (size_t i = 0; i < variables_.size(); i++) {
    Variable *var = *it;
    Token id = var->GetID();

    // Skip color variables as they will be handled by WriteColorVariables
    if (id == Token::VarColor_0 || id == Token::VarColor_1 || id == Token::VarColor_2 || id == Token::VarColor_3 ||
        id == Token::VarColor_4 || id == Token::VarColor_5 || id == Token::VarColor_6 || id == Token::VarColor_7 ||
        id == Token::VarColor_8 || id == Token::VarColor_9 || id == Token::VarColor_A || id == Token::VarColor_B ||
        id == Token::VarColor_C || id == Token::VarColor_D || id == Token::VarColor_E || id == Token::VarColor_F) {
      it++;
      continue;
    }

    etl::string<16> elemName = var->GetName();

    printer->OpenElement(elemName.c_str());
    // these settings need to be saved as the Int values not as String
    // values hence we *dont* use GetString() !
    if (var->GetType() == Variable::CHAR_LIST) {
      char buf[16];
      npf_snprintf(buf, sizeof(buf), "%d", var->GetInt());
      printer->PushAttribute(XML_ATTR_VALUE, buf);
    } else {
      // all other settings need to be saved as thier String values
      printer->PushAttribute(XML_ATTR_VALUE, var->GetString().c_str());
    }
    printer->CloseElement();
    it++;
  }

  // Write color variables using the dedicated method
  WriteColorVariables(printer);

  printer->CloseElement();
  Trace::Log("CONFIG", "Saved config");
}

bool Config::LoadTheme(PersistencyDocument *doc) {
  Trace::Log("CONFIG", "Loading theme content from XML");

  // Find the THEME root element
  if (!doc->FirstChild() || strcmp(XML_ELEM_THEME, doc->ElemName()) != 0) {
    Trace::Error("Could not find <Theme> element in document");
    return false;
  }

  // Enter the THEME element to find its children
  if (doc->FirstChild()) {
    // Process all child elements of THEME
    do {
      char *elemName = doc->ElemName();
      Trace::Log("CONFIG", "Processing element: %s", elemName);

      if (strcmp(elemName, XML_ELEM_FONT) == 0) {
        // Process Font element attributes
        while (doc->NextAttribute()) {
          if (strcmp(doc->attrname_, XML_ATTR_VALUE) == 0) {
            Trace::Log("CONFIG", "Found font value: %s", doc->attrval_);
            // Parse font value as decimal
            int fontValue = atoi(doc->attrval_);
            Trace::Log("CONFIG", "Parsed font value: %d", fontValue);

            Variable *fontVar = FindVariable(Token::VarUIFont);
            if (fontVar) {
              fontVar->SetInt(fontValue);
              Trace::Log("CONFIG", "Set font variable to: %d", fontValue);
            }
          }
        }
      } else if (strcmp(elemName, XML_ELEM_COLOR) == 0) {
        Trace::Log("CONFIG", "Found Color element");

        // Process this color element directly
        ReadColorVariable(doc);
      }
    } while (doc->NextSibling());
  }
  return true;
}

int Config::GetValue(const char *key) {
  Variable *v = FindVariable(key);
  if (v) {
    Trace::Log("CONFIG", "Got value for %s=%s", key, v->GetString().c_str());
  } else {
    Trace::Log("CONFIG", "No value for requested key:%s", key);
  }
  return v ? v->GetInt() : 0;
}

bool Config::ExportTheme(const char *themeName, bool overwrite) {
  auto fs = FileSystem::GetInstance();

  // Add .thm extension to the filename
  etl::string<MAX_THEME_NAME_LENGTH> filename = themeName;
  filename.append(THEME_FILE_EXTENSION);

  // Create themes directory if it doesn't exist
  if (!fs->exists(THEMES_DIR)) {
    Trace::Error("Expected themes directory doesn't exist!");
    return false;
  }

  // Build the full path to the theme file
  etl::string<MAX_THEME_EXPORT_PATH_LENGTH> path = THEMES_DIR;
  path.append("/");
  path.append(filename);

  // Check if the file already exists and we're not overwriting
  if (fs->exists(path.c_str()) && !overwrite) {
    Trace::Error("Theme file already exists: %s", path.c_str());
    return false;
  }

  // Open the file for writing
  auto fp = fs->Open(path.c_str(), "w");
  if (!fp) {
    Trace::Error("Failed to open theme file for writing: %s", path.c_str());
    return false;
  }

  tinyxml2::XMLPrinter printer(fp.get());

  // Use the SaveTheme method to save the theme data
  SaveTheme(&printer, themeName);

  Trace::Log("CONFIG", "Successfully exported theme to: %s", path.c_str());
  return true;
}

bool Config::ImportTheme(const char *themeName) {
  auto fs = FileSystem::GetInstance();

  // Check if the filename already has the .thm extension
  etl::string<MAX_THEME_NAME_LENGTH + strlen(THEME_FILE_EXTENSION)> filename = themeName;

  const char *extension = strrchr(themeName, '.');
  if (!extension || strcmp(extension, THEME_FILE_EXTENSION) != 0) {
    // Add .thm extension only if it's not already there
    filename.append(THEME_FILE_EXTENSION);
  }

  // Extract the theme name without extension for storing in the config
  etl::string<MAX_THEME_NAME_LENGTH> baseThemeName = themeName;
  if (extension && strcmp(extension, THEME_FILE_EXTENSION) == 0) {
    // Remove the extension from the theme name
    baseThemeName = etl::string<MAX_THEME_NAME_LENGTH>(themeName, extension - themeName);
  }

  // Build the full path to the theme file
  etl::string<MAX_THEME_EXPORT_PATH_LENGTH> path = THEMES_DIR;
  path.append("/");
  path.append(filename);

  // Sanity check if the file exists
  if (!fs->exists(path.c_str())) {
    Trace::Error("Theme file does not exist: %s", path.c_str());
    return false;
  }

  // Create a persistency document from the file
  PersistencyDocument doc;
  if (!doc.Load(path.c_str())) {
    Trace::Error("Failed to load theme document: %s", path.c_str());
    return false;
  }

  // Store the theme name in the config
  Variable *themeVar = FindVariable(Token::VarThemeName);
  themeVar->SetString(baseThemeName.c_str());

  // Use the LoadTheme method to load the theme data
  bool result = LoadTheme(&doc);

  // Save the config to persist the theme name
  if (result) {
    Save();
  }

  return result;
}
