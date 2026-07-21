cmake_minimum_required(VERSION 3.16)
project(duppy_conquerer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(duppy
  src/main.cpp
  src/ecs/EntityRegistry.cpp
  src/ecs/systems/AttachmentSystem.cpp
  src/ecs/systems/TargetSystem.cpp
  src/ecs/services/RaycastConeFactoryService.cpp
  src/terrain/PerlinNoise2D.cpp
  src/core/ConsoleInputService.cpp
  src/core/GameLoopService.cpp
  src/core/SteadyTimeSource.cpp
  src/games/AttachmentDemoGame.cpp
  src/games/TargetDemoGame.cpp
)

target_include_directories(duppy PRIVATE src)

if(MSVC)
  target_compile_options(duppy PRIVATE /W4 /permissive-)
else()
  target_compile_options(duppy PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(duppy PRIVATE Threads::Threads)
