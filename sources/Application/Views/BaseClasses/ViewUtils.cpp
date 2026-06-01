#include "ViewUtils.h"

#include "Application/AppWindow.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include <string.h>

#define LABEL_COLOR cNormal
#define VALUE_COLOR cEmphasis

void DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, bool focused) {
  ((AppWindow &)w).SetBackgroundColor(cBackground);

  // grab colon position
  char *colon = strchr(buffer, ':');

  if (colon) {
    int index = colon - buffer;
    buffer[index] = 0;

    ((AppWindow &)w).SetColor(LABEL_COLOR);
    w.DrawString(buffer, position);

    position.x_ += strlen(buffer) + 1;
    buffer += strlen(buffer) + 1;
  }

  if (focused) {
    ((AppWindow &)w).SetBackgroundColor(cHighlight2);
    ((AppWindow &)w).SetColor(cBackground);

    w.DrawString(buffer, position);
    /* todo: reenable cuttoff thingie for preciosion
    int percentPos = -1;
    for (unsigned int i = 0; i < strlen(format_); i++) {
      if (format_[i] == '%') {
        percentPos = i;
        break;
      };
    };
    if (percentPos >= 0) {
      int offset = ( 4 - position_) + percentPos; // todo: replace 4 with prevision_
      buffer[offset + 1] = 0;
      position.x_ += offset;
      ((AppWindow &)w).SetColor(cCursor); // todo: where does this happen?
      w.DrawString(buffer + offset, position);
    }
    */
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
