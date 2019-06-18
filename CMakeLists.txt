cmake_minimum_required(VERSION 3.6)

project(clumpy)

# To override, invoke cmake with -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(ASMJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/asmjit")

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(extern/blend2d)
add_subdirectory(extern/blob)
add_subdirectory(extern/cnpy)
add_subdirectory(extern/fmt)

set(CMDS
  commands/advect_points.cc
  commands/bridson_points.cc
  commands/cull_points.cc
  commands/curl_2d.cc
  commands/generate_dshapes.cc
  commands/generate_simplex.cc
  commands/gradient_noise.cc
  commands/pendulum_phase.cc
  commands/pendulum_render.cc
  commands/splat_points.cc
  commands/test_clumpy.cc
  commands/visualize_sdf.cc)

find_package(CGAL QUIET)
if (CGAL_FOUND)
    message("Found CGAL in ${CGAL_DIR}")
    set(CMDS ${CMDS} commands/cgal_streamlines.cc)
    link_directories(${CGAL_LIBRARIES_DIR})
    include_directories(${CGAL_INCLUDE_DIRS})
    link_libraries(${CGAL_LIB} gmp)
endif()

find_package(embree 3.0 QUIET PATHS /usr/lib64/cmake)
if (embree_FOUND AND !LINUX)
    message("Found embree in ${embree_DIR}")
    set(CMDS ${CMDS} commands/test_embree.cc)
    include_directories(${EMBREE_INCLUDE_DIRS})
    link_libraries(${EMBREE_LIBRARIES} tbb)
endif()

add_executable(clumpy
  ${CMDS}
  clumpy_command.hh
  main_clumpy.cc)

target_include_directories(clumpy PRIVATE extern extern/blend2d/src extern/glm .)
target_link_libraries(clumpy blend2d fmt cnpy)
