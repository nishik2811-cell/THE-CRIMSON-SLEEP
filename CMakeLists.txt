cmake_minimum_required(VERSION 3.16)
project(TheCrimsonSleep CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 2.5 / 2.6 (Graphics, Window, System, Audio)
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(crimson_sleep ${SOURCES})
target_include_directories(crimson_sleep PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(crimson_sleep PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

add_executable(ds_tests tests/ds_tests.cpp)
target_include_directories(ds_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")

# Run the game from a folder that contains assets/
add_custom_command(TARGET crimson_sleep POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:crimson_sleep>/assets")
