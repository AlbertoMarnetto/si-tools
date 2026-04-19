cmake_minimum_required(VERSION 3.10)
project(si-free-flight-2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MINGW)
    # To build a Windows release with MinGW:
    # cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../windows.toolchain.cmake
    add_link_options(-Bstatic -static-libstdc++ -static-libgcc)
endif (MINGW)

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

# Main executable
add_executable(si-free-flight-2
    main.cpp

    camera.cpp
    game_state.cpp
    icon_data.cpp
    island.cpp
    lzexe_decompress.cpp
    preview_box.cpp
    render_texture_resource.cpp
    renderer.cpp
    scene.cpp
    scenegraph_loader.cpp
    selection.cpp
    si_coords.cpp
    siobj_lib.cpp
    terrain_mapbin.cpp
    write_debug.cpp
)

target_include_directories(si-free-flight-2 PRIVATE ${RAYLIB_INCLUDE_DIR})
target_link_libraries(si-free-flight-2 PRIVATE
    raylib
)

# Enable optimizations for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(si-free-flight-2 PRIVATE -O2)
endif()
