#ifndef VIEWUTILS_H
#define VIEWUTILS_H

class GUIWindow;
struct GUIPoint;
class ViewData;

int FindFormatValueOffset(const char *format);
int DrawLabeledField(GUIWindow &w, GUIPoint position, char *buffer, bool focused = false, int subSelectionOffset = -1,
                     int subSelectionLength = 1);

bool goProjectSamplesDir(ViewData *viewData_);

#endif
