add_subdirectory(TinyXML2)
add_subdirectory(yxml)
add_subdirectory(etl)
add_subdirectory(SRC)

target_include_directories(etl INTERFACE "${PROJECT_SOURCE_DIR}/config")