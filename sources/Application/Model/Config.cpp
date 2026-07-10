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
  FourCC::enum_type fourcc;
  const char **options;
  int optionCount;
  bool isString;
};

// Define parameters as a static array instead of a ETL flat_map for example,
// because using a flat_map static requires too much stack space for
// initialization
static const ConfigParam configParams[] = {
    // Color variables
    {"Color0", {.intValue = ThemeConstants::DEFAULT_COLOR0}, FourCC::VarColor_0_Black, nullptr, 0, false},
    {"Color1", {.intValue = ThemeConstants::DEFAULT_COLOR1}, FourCC::VarColor_1_Maroon, nullptr, 0, false},
    {"Color2", {.intValue = ThemeConstants::DEFAULT_COLOR2}, FourCC::VarColor_2_Green, nullptr, 0, false},
    {"Color3", {.intValue = ThemeConstants::DEFAULT_COLOR3}, FourCC::VarColor_3_Olive, nullptr, 0, false},
    {"Color4", {.intValue = ThemeConstants::DEFAULT_COLOR4}, FourCC::VarColor_4_Blue, nullptr, 0, false},
    {"Color5", {.intValue = ThemeConstants::DEFAULT_COLOR5}, FourCC::VarColor_5_Purple, nullptr, 0, false},
    {"Color6", {.intValue = ThemeConstants::DEFAULT_COLOR6}, FourCC::VarColor_6_Turqoise, nullptr, 0, false},
    {"Color7", {.intValue = ThemeConstants::DEFAULT_COLOR7}, FourCC::VarColor_7_LightyGray},
    {"Color8", {.intValue = ThemeConstants::DEFAULT_COLOR8}, FourCC::VarColor_8_Gray, nullptr, 0, false},
    {"Color9", {.intValue = ThemeConstants::DEFAULT_COLOR9}, FourCC::VarColor_9_Red, nullptr, 0, false},
    {"Color10", {.intValue = ThemeConstants::DEFAULT_COLOR10}, FourCC::VarColor_A_Lime, nullptr, 0, false},
    {"Color11", {.intValue = ThemeConstants::DEFAULT_COLOR11}, FourCC::VarColor_B_Yellow, nullptr, 0, false},
    {"Color12", {.intValue = ThemeConstants::DEFAULT_COLOR12}, FourCC::VarColor_C_LightBlue, nullptr, 0, false},
    {"Color13", {.intValue = ThemeConstants::DEFAULT_COLOR13}, FourCC::VarColor_D_Magenta, nullptr, 0, false},
    {"Color14", {.intValue = ThemeConstants::DEFAULT_COLOR14}, FourCC::VarColor_E_Cyan, nullptr, 0, false},
    {"Color15", {.intValue = ThemeConstants::DEFAULT_COLOR15}, FourCC::VarColor_F_White, nullptr, 0, false},

    {"ThemeName", {.strValue = ThemeConstants::DEFAULT_THEME_NAME}, FourCC::VarThemeName, nullptr, 0, true},

    // Device settings with options
    {"LineOut", {.intValue = DEFAULT_LINEOUT}, FourCC::VarLineOut, lineOutOptions, 3, false},
    {"MidiDevice", {.intValue = DEFAULT_MIDIDEVICE}, FourCC::VarMidiDevice, midiDeviceList, 4, false},
    {"MidiSync", {.intValue = DEFAULT_MIDISYNC}, FourCC::VarMidiSync, midiSendSync, 2, false},
    {"mirrorUI", {.intValue = DEFAULT_REMOTEUI}, FourCC::VarMirrorUI, mirrorUIOnOff, 2, false},
    {"UIFont",
     {.intValue = ThemeConstants::DEFAULT_UIFONT},
     FourCC::VarUIFont,
     ThemeConstants::THEME_FONT_NAMES,
     ThemeConstants::THEME_FONT_COUNT,
     false},

    // Display brightness setting
    {"BacklightLevel", {.intValue = DEFAULT_BACKLIGHT_LEVEL}, FourCC::VarBacklightLevel, nullptr, 0, false},
    {"OutputVolume", {.intValue = DEFAULT_OUTPUT_VOLUME}, FourCC::VarOutputVolume, nullptr, 0, false},
    {"ImportResampling",
     {.intValue = DEFAULT_IMPORT_RESAMPLER},
     FourCC::VarImportResampler,
     importResamplerOptions,
     kImportResamplerOptionCount,
     false},
    {"UseCommandPicker",
     {.intValue = DEFAULT_USE_COMMAND_PICKER},
     FourCC::VarConfigCommandPicker,
     commandPickerOptions,
     2},
};

Config::Config()
    : VariableContainer(&variables_),
      color0_(FourCC::VarColor_0_Black, static_cast<int>(ThemeConstants::DEFAULT_COLOR0)),
      color1_(FourCC::VarColor_1_Maroon, static_cast<int>(ThemeConstants::DEFAULT_COLOR1)),
      color2_(FourCC::VarColor_2_Green, static_cast<int>(ThemeConstants::DEFAULT_COLOR2)),
      color3_(FourCC::VarColor_3_Olive, static_cast<int>(ThemeConstants::DEFAULT_COLOR3)),
      color4_(FourCC::VarColor_4_Blue, static_cast<int>(ThemeConstants::DEFAULT_COLOR4)),
      color5_(FourCC::VarColor_5_Purple, static_cast<int>(ThemeConstants::DEFAULT_COLOR5)),
      color6_(FourCC::VarColor_6_Turqoise, static_cast<int>(ThemeConstants::DEFAULT_COLOR6)),
      color7_(FourCC::VarColor_7_LightyGray, static_cast<int>(ThemeConstants::DEFAULT_COLOR7)),
      color8_(FourCC::VarColor_8_Gray, static_cast<int>(ThemeConstants::DEFAULT_COLOR8)),
      color9_(FourCC::VarColor_9_Red, static_cast<int>(ThemeConstants::DEFAULT_COLOR9)),
      color10_(FourCC::VarColor_A_Lime, static_cast<int>(ThemeConstants::DEFAULT_COLOR10)),
      color11_(FourCC::VarColor_B_Yellow, static_cast<int>(ThemeConstants::DEFAULT_COLOR11)),
      color12_(FourCC::VarColor_C_LightBlue, static_cast<int>(ThemeConstants::DEFAULT_COLOR12)),
      color13_(FourCC::VarColor_D_Magenta, static_cast<int>(ThemeConstants::DEFAULT_COLOR13)),
      color14_(FourCC::VarColor_E_Cyan, static_cast<int>(ThemeConstants::DEFAULT_COLOR14)),
      color15_(FourCC::VarColor_F_White, static_cast<int>(ThemeConstants::DEFAULT_COLOR15)),

      lineOut_(FourCC::VarLineOut, lineOutOptions, 3, DEFAULT_LINEOUT),
      midiDevice_(FourCC::VarMidiDevice, midiDeviceList, 4, DEFAULT_MIDIDEVICE),
      midiSync_(FourCC::VarMidiSync, midiSendSync, 2, DEFAULT_MIDISYNC),
      mirrorUI_(FourCC::VarMirrorUI, mirrorUIOnOff, 2, DEFAULT_REMOTEUI),
      importResampler_(FourCC::VarImportResampler, importResamplerOptions, kImportResamplerOptionCount,
                       DEFAULT_IMPORT_RESAMPLER),
      commandInputMode_(FourCC::VarConfigCommandPicker, commandPickerOptions, 2, DEFAULT_USE_COMMAND_PICKER),
      uiFont_(FourCC::VarUIFont, ThemeConstants::THEME_FONT_NAMES, ThemeConstants::THEME_FONT_COUNT,
              ThemeConstants::DEFAULT_UIFONT),
      themeName_(FourCC::VarThemeName, ThemeConstants::DEFAULT_THEME_NAME),
      backlightLevel_(FourCC::VarBacklightLevel, DEFAULT_BACKLIGHT_LEVEL),
      outputVolume_(FourCC::VarOutputVolume, DEFAULT_OUTPUT_VOLUME) {

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
        if (Variable *themeVar = FindVariable(FourCC::VarThemeName)) {
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
    FourCC id = var->GetID();

    // Check if this is a color variable
    if (id == FourCC::VarColor_0_Black || id == FourCC::VarColor_1_Maroon || id == FourCC::VarColor_2_Green ||
        id == FourCC::VarColor_3_Olive || id == FourCC::VarColor_4_Blue || id == FourCC::VarColor_5_Purple ||
        id == FourCC::VarColor_6_Turqoise || id == FourCC::VarColor_7_LightyGray || id == FourCC::VarColor_8_Gray ||
        id == FourCC::VarColor_9_Red || id == FourCC::VarColor_A_Lime || id == FourCC::VarColor_B_Yellow ||
        id == FourCC::VarColor_C_LightBlue || id == FourCC::VarColor_D_Magenta || id == FourCC::VarColor_E_Cyan ||
        id == FourCC::VarColor_F_White) {

      // Open a Color element
      printer->OpenElement(XML_ELEM_COLOR);

      // Add name attribute
      printer->PushAttribute(XML_ATTR_NAME, var->GetName());

      // Format color value in hex format with # prefix
      char hexValue[16];
      npf_snprintf(hexValue, sizeof(hexValue), "#%X", var->GetInt());

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
        FourCC fourcc = FourCC::Default; // Use Default as invalid marker

        if (strcmp(colorName, "Color0") == 0) {
          fourcc = FourCC::VarColor_0_Black;
        } else if (strcmp(colorName, "Color1") == 0) {
          fourcc = FourCC::VarColor_1_Maroon;
        } else if (strcmp(colorName, "Color2") == 0) {
          fourcc = FourCC::VarColor_2_Green;
        } else if (strcmp(colorName, "Color3") == 0) {
          fourcc = FourCC::VarColor_3_Olive;
        } else if (strcmp(colorName, "Color4") == 0) {
          fourcc = FourCC::VarColor_4_Blue;
        } else if (strcmp(colorName, "Color5") == 0) {
          fourcc = FourCC::VarColor_5_Purple;
        } else if (strcmp(colorName, "Color6") == 0) {
          fourcc = FourCC::VarColor_6_Turqoise;
        } else if (strcmp(colorName, "Color7") == 0) {
          fourcc = FourCC::VarColor_7_LightyGray;
        } else if (strcmp(colorName, "Color8") == 0) {
          fourcc = FourCC::VarColor_8_Gray;
        } else if (strcmp(colorName, "Color9") == 0) {
          fourcc = FourCC::VarColor_9_Red;
        } else if (strcmp(colorName, "Color9") == 0) {
          fourcc = FourCC::VarColor_9_Red;
        } else if (strcmp(colorName, "Color10") == 0) {
          fourcc = FourCC::VarColor_A_Lime;
        } else if (strcmp(colorName, "Color11") == 0) {
          fourcc = FourCC::VarColor_B_Yellow;
        } else if (strcmp(colorName, "Color12") == 0) {
          fourcc = FourCC::VarColor_C_LightBlue;
        } else if (strcmp(colorName, "Color13") == 0) {
          fourcc = FourCC::VarColor_D_Magenta;
        } else if (strcmp(colorName, "Color14") == 0) {
          fourcc = FourCC::VarColor_E_Cyan;
        } else if (strcmp(colorName, "Color15") == 0) {
          fourcc = FourCC::VarColor_F_White;
        }

        if (fourcc != FourCC::Default) { // If we found a valid color
          Variable *var = FindVariable(fourcc);
          if (var) {
            var->SetInt(value);
            Trace::Log("CONFIG", "Read Color: %s = %d", colorName, value);
          }
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
  Variable *fontVar = FindVariable(FourCC::VarUIFont);
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
    FourCC id = var->GetID();

    // Skip color variables as they will be handled by WriteColorVariables
    if (id == FourCC::VarColor_0_Black || id == FourCC::VarColor_1_Maroon || id == FourCC::VarColor_2_Green ||
        id == FourCC::VarColor_3_Olive || id == FourCC::VarColor_4_Blue || id == FourCC::VarColor_5_Purple ||
        id == FourCC::VarColor_6_Turqoise || id == FourCC::VarColor_7_LightyGray || id == FourCC::VarColor_8_Gray ||
        id == FourCC::VarColor_9_Red || id == FourCC::VarColor_A_Lime || id == FourCC::VarColor_B_Yellow ||
        id == FourCC::VarColor_C_LightBlue || id == FourCC::VarColor_D_Magenta || id == FourCC::VarColor_E_Cyan ||
        id == FourCC::VarColor_F_White) {
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

            Variable *fontVar = FindVariable(FourCC::VarUIFont);
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
  Variable *themeVar = FindVariable(FourCC::VarThemeName);
  themeVar->SetString(baseThemeName.c_str());

  // Use the LoadTheme method to load the theme data
  bool result = LoadTheme(&doc);

  // Save the config to persist the theme name
  if (result) {
    Save();
  }

  return result;
}
