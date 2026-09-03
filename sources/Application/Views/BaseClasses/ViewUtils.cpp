#include "ViewUtils.h"

#include "Application/AppWindow.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include <string.h>

int FindFormatValueOffset(const char *format) {
  for (unsigned int i = 0; i < strlen(format); i++) {
    if (format[i] == '%') {
      return i;
    }
  }
  return -1;
}

int DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, bool focused, int subSelectionOffset,
                     int subSelectionLength) {
  ((AppWindow &)w).SetBackgroundColor(Theme::View::bg);
  ((AppWindow &)w).SetColor(Theme::View::fg);

  GUIPoint basePosition = position;

  char *colon = strchr(buffer, ':');
  int valueOffset = 0;
  char *value = buffer;

  if (colon) {
    int labelLength = (int)(colon - buffer);

    // Temporarily terminate the label.
    *colon = '\0';

    ((AppWindow &)w).SetColor(Theme::Input::label);
    w.DrawString(position.x_, position.y_, buffer);

    // Restore the caller's buffer.
    *colon = ':';

    valueOffset = labelLength + 1;
    position.x_ += valueOffset;
    value = colon + 1;
  }

  const int valueLength = static_cast<int>(strlen(value));

  if (focused) {
    ((AppWindow &)w).SetBackgroundColor(Theme::Input::bg(true));
    ((AppWindow &)w).SetColor(Theme::Input::fg(true));

    w.DrawString(position.x_, position.y_, value);

    int valueSubSelectionOffset = subSelectionOffset - valueOffset;

    if (subSelectionOffset >= valueOffset && valueSubSelectionOffset < valueLength && subSelectionLength > 0) {
      if (valueSubSelectionOffset + subSelectionLength > valueLength) {
        subSelectionLength = valueLength - valueSubSelectionOffset;
      }

      char replaced = value[valueSubSelectionOffset + subSelectionLength];

      value[valueSubSelectionOffset + subSelectionLength] = '\0';

      position.x_ += valueSubSelectionOffset;

      ((AppWindow &)w).SetBackgroundColor(Theme::Input::cursor);
      ((AppWindow &)w).SetColor(Theme::Input::fg(true));

      w.DrawString(position.x_, position.y_, value + valueSubSelectionOffset);

      value[valueSubSelectionOffset + subSelectionLength] = replaced;
    }
  } else {
    ((AppWindow &)w).SetColor(Theme::Input::fg(false));
    w.DrawString(position.x_, position.y_, value);
  }

  // draw highlight button ends
  char front = focused ? CHAR(char_button_border_left_s) : ' ';
  char end = focused ? CHAR(char_button_border_right_s) : ' ';

  if (focused) {
    ((AppWindow &)w).SetColor(Theme::Input::bg(true));
    ((AppWindow &)w).SetBackgroundColor(Theme::View::bg);
  }

  w.DrawChar(basePosition.x_ + valueOffset - 1, basePosition.y_, front);
  w.DrawChar(basePosition.x_ + strlen(buffer), basePosition.y_, end);

  return valueLength + 2;
}

bool goProjectSamplesDir(ViewData *viewData_) {
  auto fs = FileSystem::GetInstance();
  fs->chdir(PROJECTS_DIR);
  // Then, navigate into the current project's directory
  if (viewData_ && viewData_->project_) {
    char projectName[MAX_PROJECT_NAME_LENGTH + 1];
    viewData_->project_->GetProjectName(projectName);

    if (fs->chdir(projectName)) {
      // Finally, navigate into the samples subdirectory
      return fs->chdir(PROJECT_SAMPLES_DIR);
    } else {
      Trace::Error("SampleEditorView: Failed to chdir to project dir: %s", projectName);
      // Return to a known base state
      fs->chdir(SD_BASE_DIR);
      return false; // Abort if we can't find the project directory
    }
  } else {
    Trace::Error("SampleEditorView: No project data available to find samples dir.");
    fs->chdir(SD_BASE_DIR);
    return false; // Abort if project data is missing
  }
  return true;
}
