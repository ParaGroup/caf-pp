cmake_minimum_required(VERSION 3.0.0)
project(parco19_exp VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++17")

# add debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")

# JUST FOR DEBUG
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# add stdlib to clang++
# TODO: check that the libc++ is installed
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wsign-compare")
  message(STATUS "is clang, libc++ added")
endif()

# find CAF
find_library(CAF_LIB caf_core_static)
find_path(CAF_INCLUDE caf)
if(NOT CAF_LIB OR NOT CAF_INCLUDE)
  message(FATAL_ERROR "CAF library not found")
endif()
find_library(PTHREAD pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)


# Targets
add_library(caf_pp libcaf_pp/src/patterns.cpp libcaf_pp/src/policy.cpp libcaf_pp/src/spawn.cpp)
target_include_directories(caf_pp PUBLIC "${CAF_INCLUDE}")
target_link_libraries(caf_pp "${CAF_LIB}" pthread)

add_executable (pattern1 examples/pattern1.cpp)
target_include_directories(pattern1 PUBLIC "${CAF_INCLUDE}" libcaf_pp)
target_link_libraries(pattern1 caf_pp "${CAF_LIB}")
