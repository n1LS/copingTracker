add_library(application_model_generators
  DrumPhraseGenerator.cpp
  PhraseGenerator.cpp
)

target_include_directories(application_model_generators PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR})
