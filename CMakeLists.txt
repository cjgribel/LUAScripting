cmake_minimum_required(VERSION 3.14)

#
# Project
#
project(LuaScripting
  VERSION 1.0
  LANGUAGES CXX
)

message(STATUS "Starting CMake configuration")

#
# Set compiler version
# TODO: uninstall this compiler
#set(CMAKE_C_COMPILER "/opt/homebrew/Cellar/llvm@15/15.0.7/bin/clang-15")
#set(CMAKE_CXX_COMPILER "/opt/homebrew/Cellar/llvm@15/15.0.7/bin/clang++")

#
# CPP
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

#
# Compiler
#
if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION=1)
    message(STATUS "Apple platform detected, setting GL_SILENCE_DEPRECATION=1")
else()
    add_compile_definitions(GL_SILENCE_DEPRECATION=0)
    message(STATUS "Non-Apple platform detected")
endif()

# 'target_include_directories' if target specific
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/vec)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/meta)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/editor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/entt-meets-sol2)

#
# OpenGL
#
find_package(OpenGL REQUIRED)
message(STATUS "Found OpenGL")
message(STATUS "OpenGL include dir: ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL libraries: ${OPENGL_LIBRARIES}")

#
# Git
#
find_package (Git REQUIRED)
message(STATUS "Found Git: ${GIT_EXECUTABLE}")

#
# Prepare FetchContent
#
include(FetchContent)

#
# Fetch SDL
#
find_package(SDL2 QUIET)
if (NOT SDL2_FOUND)
    message(STATUS "SDL2 not found, using FetchContent to fetch it.")
    FetchContent_Declare(
        SDL2
        #URL https://www.libsdl.org/release/SDL2-2.0.12.zip
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG        v2.30.3
    )
    FetchContent_MakeAvailable(SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2)
    set(SDL2_INCLUDE_DIRS ${SDL2_SOURCE_DIR}/include)
else()
    message(STATUS "Found SDL2.")
    set(SDL2_LIBRARIES SDL2::SDL2)
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
endif()
include_directories(${SDL2_INCLUDE_DIRS})
#message(STATUS "Fetching SDL2")
#FetchContent_Declare(
#    sdl2
#    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
#    GIT_TAG        release-2.30.2 # April 2nd 2024
#)
#FetchContent_GetProperties(sdl2)
#if(NOT sdl2_POPULATED)
#    FetchContent_Populate(sdl2)
#    add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR})
#    message(STATUS "SDL2 populated and added to the build")
#endif()
#include_directories(${sdl2_SOURCE_DIR}/include)

# Fetch SDL_mixer
#
find_package(SDL2_mixer REQUIRED)
message(STATUS "Found SDL2_mixer")
#
#FetchContent_Declare(
#  SDL2_mixer
#  GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
#  GIT_TAG release-2.8.0  # Replace with the desired version
#)
#FetchContent_GetProperties(SDL2_mixer)
#if(NOT SDL2_mixer_POPULATED)
#  FetchContent_Populate(SDL2_mixer)
#
#  # Add SDL2_mixer with internal dependencies
#  add_subdirectory(${SDL2_mixer_SOURCE_DIR} ${SDL2_mixer_BINARY_DIR} EXCLUDE_FROM_ALL)
#  set(SDL2MIXER_LIBRARIES SDL2_mixer::SDL2_mixer)
#
#  # Force SDL2_mixer to use internal dependencies
#  set(OPTION_USE_SYSTEM_OPUS OFF CACHE BOOL "" FORCE)
#  set(OPTION_USE_SYSTEM_LIBXMP OFF CACHE BOOL "" FORCE)
#  set(OPTION_USE_SYSTEM_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
#  set(OPTION_USE_SYSTEM_WAVPACK OFF CACHE BOOL "" FORCE)
#endif()

#
# Fetch glew
# https://github.com/Perlmint/glew-cmake/blob/master/glew-cmake/fetch-content.cmake
#
message(STATUS "Fetching GLEW")
include(FetchContent)
FetchContent_Declare(
  glew
  GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
  GIT_TAG        origin/master
)
FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
  FetchContent_Populate(glew)
  add_subdirectory(${glew_SOURCE_DIR} ${glew_BINARY_DIR} EXCLUDE_FROM_ALL)
  message(STATUS "GLEW populated and added to the build")
endif()

#
# Fetch enTT (header-only)
#
# Release notes for 3.14
# Note deprecation for meta properties: should use 'traits' or 'custom' instead
# https://github.com/skypjack/entt/releases/tag/v3.14.0
# https://github.com/skypjack/entt/blob/master/docs/md/meta.md#user-defined-data
message(STATUS "Fetching enTT")
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.13.2
)
FetchContent_MakeAvailable(entt)
include_directories(${entt_SOURCE_DIR}/src) 
message(STATUS "enTT fetched and included")

#
# Fetch Dear ImGui
#
message(STATUS "Fetching Dear ImGui")
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.89.9)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)
message(STATUS "Dear ImGui fetched and included")

#
# Lua
#
set(LUA_VERSION v5.4.6)
find_package(Lua QUIET)
if(NOT LUA_FOUND)
    set(LUA_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG ${LUA_VERSION}  # Use the defined version variable
    )
    FetchContent_MakeAvailable(lua)
    message(STATUS "Fetched Lua ${LUA_VERSION}")
else()
    message(STATUS "Lua version ${LUA_VERSION} found, skipping fetch")
endif()
include_directories(${LUA_INCLUDE_DIR})
message(STATUS "Lua include dir: ${lua_SOURCE_DIR}")

#
# sol2
#
set(SOL2_VERSION v3.3.0)
find_package(sol2 QUIET)
if(NOT sol2_FOUND)
    set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG main #${SOL2_VERSION}  # Use the defined version variable
    )
    FetchContent_MakeAvailable(sol2)
    message(STATUS "Fetched sol2 ${SOL2_VERSION}")
else()
    message(STATUS "sol2 version ${sol2_VERSION} found, skipping fetch")
endif()
include_directories(${sol2_SOURCE_DIR}/include)
message(STATUS "sol2 include dir: ${sol2_SOURCE_DIR}")

#
# nlohmann
#
# Fetch nlohmann/json library
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

#
# Targets
#

# Module1
message(STATUS "Creating executable target for LuaGame")
add_executable(LuaGame 
    LuaGame/main.cpp
    LuaGame/Scene.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/LuaGame/GuiCommands.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp # Widgets for std::string
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/BatchLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/meta/MetaInspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/meta/MetaClone.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/meta/MetaSerialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/GuiCommands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/EditComponentCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ChunkRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GLDebugMessageCallback.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ShapeRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ParticleBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SceneGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BehaviorScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CoreComponents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec/vec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec/mat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec/quat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vec/ray.cpp
    )

set_target_properties(LuaGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/LuaGame"
)

# GLEW headers and library
# We need to prevent glew from using '-framework AGL', which was removed 
# in macOS Tahoe. The current fix is to 1) explicitly include glew headers,
# and 2) link to the glew library file directly, rather than the glew target.
# Should work equally well on all platforms.
# https://developer.apple.com/documentation/macos-release-notes/macos-26-release-notes
target_include_directories(LuaGame PRIVATE
    ${glew_SOURCE_DIR}/include
)

target_link_libraries(
    LuaGame PRIVATE 
    ${SDL2_LIBRARIES} 
    SDL2_mixer::SDL2_mixer 
    # libglew_static 
    $<TARGET_FILE:libglew_static> # link to library file directly to avoid AGL framework
    ${OPENGL_LIBRARIES} 
    ${LUA_LIBRARIES} 
    sol2::sol2
    EnTT::EnTT
    nlohmann_json::nlohmann_json)
#target_include_directories(Module1 PRIVATE ${imgui_SOURCE_DIR})
#target_include_directories(Module1 PRIVATE ${imgui_SOURCE_DIR}/backends)

# Post-build commands
add_custom_command(TARGET LuaGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:LuaGame>
)
message(STATUS "Post-build commands to copy SDL2 DLLs to Module1 folder")

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(TARGET LuaGame PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "Set Visual Studio debugger working directory")
endif()

# ModulOther targets ...

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Module1)
    message(STATUS "Set Visual Studio startup project to Module1")
endif()

# For 'make install'
#install(TARGETS Module1 Module2)
# message(STATUS "Install targets