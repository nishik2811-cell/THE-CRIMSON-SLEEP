cmake_minimum_required(VERSION 3.16)
project(TheCrimsonSleep CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3 ships CMake config files, so find_package works directly once
# libsfml-dev (or an equivalent SFML 3 install) is present on the system.
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(TheCrimsonSleep ${SOURCES})

target_include_directories(TheCrimsonSleep PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(TheCrimsonSleep PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

# Copy the assets folder (fonts) next to the built executable so the
# game can find them at runtime regardless of the build directory.
add_custom_command(TARGET TheCrimsonSleep POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:TheCrimsonSleep>/assets"
)
