add_library(platform_audio
  HostAudio.cpp
  HostAudioDriver.cpp
)

target_link_libraries(platform_audio PUBLIC
  services_audio
  application_audio
  application_player
  application_mixer
  SDL2::SDL2
)
target_include_directories(platform_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
