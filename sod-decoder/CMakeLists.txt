# SOD Parser Library and Tools
cmake_minimum_required(VERSION 3.10)
project(sod_decoder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MINGW)
    # To build a Windows release with MinGW:
    # cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../windows.toolchain.cmake
    add_link_options(-Bstatic -static-libstdc++ -static-libgcc)
endif (MINGW)

# Static library with all parsing logic
add_library(siobj_lib 
    STATIC 
    siobj_lib.cpp 
    write_debug.cpp)
target_include_directories(siobj_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# sod_decode - command-line tool using the library
add_executable(sod-decoder sod_decode.cpp)
target_link_libraries(sod-decoder siobj_lib)

# test_res_bounds - test for boundary conditions
add_executable(test-res-bounds test_res_bounds.cpp)
target_link_libraries(test-res-bounds siobj_lib)

# res_extract - extract resources from RES files
add_executable(res-extract res_extract.cpp)
target_link_libraries(res-extract siobj_lib)
