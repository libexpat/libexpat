cmake_minimum_required(VERSION 3.10.0)

project(msvc_debugging)

include(CheckCCompilerFlag)

check_c_compiler_flag("-fno-strict-aliasing" FLAG_NO_STRICT_ALIASING)
check_c_compiler_flag("-fvisibility=hidden" FLAG_VISIBILITY)

message(STATUS "CMAKE_C_COMPILER: <${CMAKE_C_COMPILER}>")
message(STATUS "CMAKE_CXX_COMPILER: <${CMAKE_CXX_COMPILER}>")
message(STATUS "CMAKE_VERSION: <${CMAKE_VERSION}>")
message(STATUS "MSVC: <${MSVC}>")
message(STATUS "MSVC_VERSION: <${MSVC_VERSION}>")

message(STATUS "FLAG_NO_STRICT_ALIASING: <${FLAG_NO_STRICT_ALIASING}>")
message(STATUS "FLAG_VISIBILITY: <${FLAG_VISIBILITY}>")
