cmake_minimum_required(VERSION 3.10)
project(tesla)

set(CMAKE_CXX_STANDARD 17)

# Set the source directory
include_directories(
    src
    include  # Add this!
)

# Gather all source files
set(SOURCES
    src/calendar.cpp
    src/carblock.cpp
    src/logger.cpp
    src/main.cpp
    src/settings.cpp
)

add_executable(tcs.out ${SOURCES})

# Link against required libraries
target_link_libraries(tcs.out
    curl
    pthread
)

add_custom_command(TARGET tcs.out POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/settings.json
    $<TARGET_FILE_DIR:tcs.out>/settings.json
)