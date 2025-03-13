cmake_minimum_required(VERSION 3.0)

set(MY_CURRENT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(CLANG_COMPILER_PATH "${MY_CURRENT_DIRECTORY}/llvm-project/prefix/bin/clang++")

message(STATUS "Clang compiler path: ${CLANG_COMPILER_PATH}")

set(CMAKE_CXX_COMPILER ${CLANG_COMPILER_PATH})

project(impact_analysis)

set(ZLIB_LIBRARY "/usr/lib/x86_64-linux-gnu/libz.so")
set(ZLIB_INCLUDE_DIR "/usr/include")

add_compile_options(-std=c++17)

include_directories(${ZLIB_INCLUDE_DIR})

set(CMAKE_BUILD_TYPE "Debug")

set(LLVM_DIR "${MY_CURRENT_DIRECTORY}/llvm-project/prefix/lib/cmake/llvm")

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
link_libraries(${llvm_libs})

add_executable(impact_analysis ${MY_CURRENT_DIRECTORY}/impact/static_analysis/main.cpp ${MY_CURRENT_DIRECTORY}/impact/static_analysis/module_parse.cpp ${MY_CURRENT_DIRECTORY}/impact/static_analysis/cfg.cpp ${MY_CURRENT_DIRECTORY}/impact/static_analysis/dfg.cpp ${MY_CURRENT_DIRECTORY}/impact/static_analysis/sdg.cpp)
target_link_libraries(impact_analysis PRIVATE ${ZLIB_LIBRARY})