cmake_minimum_required(VERSION 3.10)
project(LavaLampScreensaver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Add executable
add_executable(lavalamp
    src/main.cpp
    src/Molecule.cpp
    src/LavaLamp.cpp
    src/Renderer.cpp
    src/Blob.cpp

)

target_link_libraries(lavalamp ${SDL2_LIBRARIES})
