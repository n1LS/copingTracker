add_library(application_views_baseclasses
../ScreenView.cpp
  ListView.cpp
  FileListView.cpp
  I_Action.cpp
  ModalView.cpp
  UIActionField.cpp
  UIBitmaskVarField.cpp
  UIField.cpp
  UIHexVarField.cpp
  UIIntField.cpp
  UIIntVarField.cpp
  UIIntVarOffField.cpp
  UINoteVarField.cpp
  UISortedVarList.cpp
  UIStaticField.cpp
  UISwatchField.cpp
  UITempoField.cpp
  UITabField.cpp
  UITextField.cpp
  View.cpp
  ViewEvent.cpp
  ViewUtils.cpp
)

target_link_libraries(application_views_baseclasses PUBLIC
  application_utils
  system_console
)

target_include_directories(application_views_baseclasses PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

include_directories(${PROJECT_SOURCE_DIR})
