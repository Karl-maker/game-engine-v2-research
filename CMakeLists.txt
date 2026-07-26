cmake_minimum_required(VERSION 3.16)
project(duppy_conquerer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(duppy
  src/main.cpp
  src/data/Json.cpp
  src/ecs/EntityRegistry.cpp
  src/assets/MeshAssetService.cpp
  src/ecs/systems/AnimationSystem.cpp
  src/ecs/systems/AudioSystem.cpp
  src/ecs/systems/AttachmentSystem.cpp
  src/ecs/systems/CollisionDetectionSystem.cpp
  src/ecs/systems/CollisionResolutionSystem.cpp
  src/ecs/systems/ControllerSystem.cpp
  src/ecs/systems/GraphicsSystem.cpp
  src/ecs/systems/GravitySystem.cpp
  src/ecs/systems/HierarchySystem.cpp
  src/ecs/systems/IdleAnimationSystem.cpp
  src/ecs/systems/IKSystem.cpp
  src/ecs/systems/JumpSystem.cpp
  src/ecs/systems/HitDetectionSystem.cpp
  src/ecs/systems/MotionSystem.cpp
  src/ecs/systems/MovementSystem.cpp
  src/ecs/systems/HudSystem.cpp
  src/ecs/systems/PoseSystem.cpp
  src/ecs/systems/RayDetectionSystem.cpp
  src/ecs/systems/SensorSystem.cpp
  src/ecs/systems/SkeletonAssetSyncSystem.cpp
  src/ecs/systems/SkyPresetSystem.cpp
  src/ecs/systems/SocketSystem.cpp
  src/ecs/systems/TargetSystem.cpp
  src/ecs/systems/ThirdPersonCameraSystem.cpp
  src/ecs/factories/ObjectFactory.cpp
  src/ecs/factories/PhysicalObjectFactory.cpp
  src/ecs/factories/ActorFactory.cpp
  src/ecs/factories/WeaponFactory.cpp
  src/ecs/factories/CombatantFactory.cpp
  src/ecs/factories/FactoryKeyService.cpp
  src/ecs/services/IKService.cpp
  src/ecs/services/ChunkStreamingService.cpp
  src/ecs/services/FileChunkSource.cpp
  src/ecs/services/RaycastConeFactoryService.cpp
  src/ecs/services/SpatialHashGridService.cpp
  src/terrain/PerlinNoise2D.cpp
  src/core/ConsoleInputService.cpp
  src/core/ControlService.cpp
  src/core/GameLoopService.cpp
  src/core/SteadyTimeSource.cpp
  src/core/ThreadService.cpp
  src/games/AttachmentDemoGame.cpp
  src/games/GameplayDemoGame.cpp
  src/games/TargetDemoGame.cpp
)

target_include_directories(duppy PRIVATE src)

if(MSVC)
  target_compile_options(duppy PRIVATE /W4 /permissive-)
else()
  target_compile_options(duppy PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT DUPPY_IPO_SUPPORTED OUTPUT DUPPY_IPO_OUTPUT)
if(DUPPY_IPO_SUPPORTED)
  set_property(TARGET duppy PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  set_property(TARGET duppy PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()

find_package(Threads REQUIRED)
target_link_libraries(duppy PRIVATE Threads::Threads)

# --- OpenGL demo renderer (optional) ---
option(DUPPY_ENABLE_OPENGL "Enable GLFW/OpenGL rendering for the gameplay demo" ON)
if(DUPPY_ENABLE_OPENGL)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(GLFW QUIET glfw3)
  endif()

  if(GLFW_FOUND)
    target_sources(duppy PRIVATE
      src/graphics/OpenGlRenderer.cpp
      src/graphics/ShaderService.cpp
      src/graphics/TextureService.cpp
    )
    target_compile_definitions(duppy PRIVATE DUPPY_ENABLE_OPENGL=1)
    target_include_directories(duppy PRIVATE ${GLFW_INCLUDE_DIRS})
    target_link_directories(duppy PRIVATE ${GLFW_LIBRARY_DIRS})
    target_link_libraries(duppy PRIVATE ${GLFW_LIBRARIES})
    target_compile_options(duppy PRIVATE ${GLFW_CFLAGS_OTHER})
    target_link_options(duppy PRIVATE ${GLFW_LDFLAGS_OTHER})

    if(APPLE)
      target_link_libraries(duppy PRIVATE "-framework OpenGL")
      target_link_libraries(duppy PRIVATE "-framework ImageIO" "-framework CoreGraphics" "-framework CoreFoundation")
    else()
      find_package(OpenGL REQUIRED)
      target_link_libraries(duppy PRIVATE OpenGL::GL)
    endif()
  else()
    target_compile_definitions(duppy PRIVATE DUPPY_ENABLE_OPENGL=0)
  endif()
else()
  target_compile_definitions(duppy PRIVATE DUPPY_ENABLE_OPENGL=0)
endif()
