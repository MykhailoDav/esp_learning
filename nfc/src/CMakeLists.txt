# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
FILE(GLOB_RECURSE pn532_sources ${CMAKE_SOURCE_DIR}/lib/pn532/*.c)

idf_component_register(
    SRCS ${app_sources} ${pn532_sources}
    INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/pn532
)

