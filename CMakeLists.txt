cmake_minimum_required(VERSION 3.16)
project(omp_screensaver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenMP (opcional pero recomendado)
find_package(OpenMP)
if (OpenMP_CXX_FOUND)
  message(STATUS "OpenMP found")
endif()

# Intento 1: SDL2 vía config (vcpkg/Windows)
find_package(SDL2 CONFIG QUIET)

if (NOT SDL2_FOUND)
  # Intento 2: pkg-config (Linux/WSL)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(SDL2 REQUIRED sdl2)
endif()

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/App.cpp
  src/Args.cpp
  src/Color.cpp
  src/Particle.cpp
  src/Timer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  include
)

if (SDL2_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
else()
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()

if (OpenMP_CXX_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENMP=1)
endif()
