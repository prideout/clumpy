cmake_minimum_required(VERSION 3.6)

project(clumpy)

# To override, invoke cmake with -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(extern/blob)
add_subdirectory(extern/cnpy)
add_subdirectory(extern/fmt)
add_subdirectory(extern/svg)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

include_directories(extern extern/glm)

set(SOURCE
  bridson_points.cc
  clumpy_command.hh
  curl_2d.cc
  generate_dshapes.cc
  generate_simplex.cc
  main_clumpy.cc
  splat_points.cc
  test_clumpy.cc
  visualize_sdf.cc)

link_libraries(fmt cnpy)

add_executable(clumpy ${SOURCE})
