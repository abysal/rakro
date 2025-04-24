cmake_minimum_required(VERSION 3.16)
project(rakro_root LANGUAGES CXX)

# Set default C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the main library
add_subdirectory(rakro)

# Optionally add the playground
option(CONET_BUILD_PLAYGROUND "Build the rakro_playground executable" ON)
if(CONET_BUILD_PLAYGROUND)
    add_subdirectory(playground)
endif()