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
