cmake_minimum_required(VERSION 3.20)
project(double-go VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main library
add_library(double-go-lib STATIC src/board.cpp)
target_include_directories(double-go-lib PUBLIC include)

# Executable
add_executable(double-go src/main.cpp)
target_link_libraries(double-go PRIVATE double-go-lib)

# GUI (requires SDL2)
find_package(SDL2 REQUIRED)
add_executable(double-go-gui src/gui.cpp)
target_link_libraries(double-go-gui PRIVATE double-go-lib SDL2::SDL2 SDL2::SDL2main)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
