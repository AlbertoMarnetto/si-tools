# SOD Parser Library and Tools
cmake_minimum_required(VERSION 3.10)
project(sod_parser CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static library with all parsing logic
add_library(siobj_lib 
    STATIC 
    siobj_lib.cpp 
    write_debug.cpp)
target_include_directories(siobj_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# sod_decode - command-line tool using the library
add_executable(sod_decode sod_decode.cpp)
target_link_libraries(sod_decode siobj_lib)

# test_res_bounds - test for boundary conditions
add_executable(test_res_bounds test_res_bounds.cpp)
target_link_libraries(test_res_bounds siobj_lib)

# res_extract - extract resources from RES files
add_executable(res_extract res_extract.cpp)
target_link_libraries(res_extract siobj_lib)
