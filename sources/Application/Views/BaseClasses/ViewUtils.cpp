#include "ViewUtils.h"

#include "Application/AppWindow.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include <string.h>

#define LABEL_COLOR cNormal
#define VALUE_COLOR cEmphasis

int FindFormatValueOffset(const char *format) {
  for (unsigned int i = 0; i < strlen(format); i++) {
    if (format[i] == '%') {
      return i;
    }
  }
  return -1;
}

void DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, bool focused, int subSelectionOffset,
                      int subSelectionLength) {
  ((AppWindow &)w).SetBackgroundColor(cBackground);

  // grab colon position
  char *colon = strchr(buffer, ':');
  int valueOffset = 0;

  if (colon) {
    int index = colon - buffer;
    buffer[index] = 0;
    valueOffset = index + 1;

    ((AppWindow &)w).SetColor(LABEL_COLOR);
    w.DrawString(buffer, position);

    position.x_ += strlen(buffer) + 1;
    buffer += strlen(buffer) + 1;
  }

  if (focused) {
    ((AppWindow &)w).SetBackgroundColor(cHighlight2);
    ((AppWindow &)w).SetColor(cBackground);

    w.DrawString(buffer, position);

    int valueSubSelectionOffset = subSelectionOffset - valueOffset;
    if (subSelectionOffset >= valueOffset && valueSubSelectionOffset < (int)strlen(buffer) && subSelectionLength > 0) {
      int valueLength = strlen(buffer);
      if (valueSubSelectionOffset + subSelectionLength > valueLength) {
        subSelectionLength = valueLength - valueSubSelectionOffset;
      }

      char replaced = buffer[valueSubSelectionOffset + subSelectionLength];
      buffer[valueSubSelectionOffset + subSelectionLength] = 0;
      position.x_ += valueSubSelectionOffset;
      ((AppWindow &)w).SetBackgroundColor(cNormal);
      w.DrawString(buffer + valueSubSelectionOffset, position);
      buffer[valueSubSelectionOffset + subSelectionLength] = replaced;
    }
  } else {
    ((AppWindow &)w).SetColor(VALUE_COLOR);
    w.DrawString(buffer, position);
  }
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
      // It's good practice to return to the root to avoid being in an unknown
      // state
      fs->chdir("/");
      return false; // Abort if we can't find the project directory
    }
  } else {
    Trace::Error("SampleEditorView: No project data available to find samples dir.");
    fs->chdir("/");
    return false; // Abort if project data is missing
  }
  return true;
}
