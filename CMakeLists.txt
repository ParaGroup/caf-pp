cmake_minimum_required(VERSION 3.0.0)
project(caf-pp VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++17")

# add debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")

# add stdlib to clang++
# TODO: check that the libc++ is installed
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wsign-compare")
  message(STATUS "is clang, libc++ added")
endif()

# find CAF
find_library(CAF_LIB caf_core_static)
find_path(CAF_INCLUDE caf/all.hpp)
if(NOT CAF_LIB OR NOT CAF_INCLUDE)
  message(FATAL_ERROR "CAF library not found")
endif()
find_library(PTHREAD pthread)

# Find/Download Range-v3
# reference: https://foonathan.net/2016/07/cmake-dependency-handling/
find_path(RANGE_INCLUDE range/v3/all.hpp)
if(RANGE_INCLUDE)
  add_library(range INTERFACE)
  target_include_directories(range INTERFACE ${RANGE_INCLUDE})
else()
  message("Unable to find 'range-v3', will be downloaded...")
  execute_process(COMMAND git submodule update --init -- external/range-v3
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  # set RANGE_INCLUDE properly
  set(RANGE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/external/range-v3/include)

  # also install it
  install(DIRECTORY ${RANGE_INCLUDE}/range DESTINATION include)

  # for convenience setup a target
  add_library(range INTERFACE)
  target_include_directories(range INTERFACE
                            $<BUILD_INTERFACE:${RANGE_INCLUDE}>
                            $<INSTALL_INTERFACE:include/range>)
  
  # need to export target as well
  install(TARGETS range EXPORT my_export_set DESTINATION include)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

# Targets
add_library(caf_pp src/pp_impl/dac.cpp
                   src/pp_impl/map.cpp
                   src/pp_impl/map2.cpp
                   src/utils/ns_type.cpp
                   src/utils/spawn_actor.cpp
                   src/patterns.cpp
                   src/policy.cpp
                   src/pp_actor.cpp
                   src/spawn.cpp
)
target_include_directories(caf_pp PUBLIC ./src ${CAF_INCLUDE} ${RANGE_INCLUDE})
target_link_libraries(caf_pp ${CAF_LIB} pthread)

add_executable (pipe-of-farm examples/pipe-of-farm.cpp)
target_include_directories(pipe-of-farm PUBLIC ${CAF_INCLUDE} ${RANGE_INCLUDE} ./src)
target_link_libraries(pipe-of-farm caf_pp ${CAF_LIB})

add_executable (pipe-of-map examples/pipe-of-map.cpp)
target_include_directories(pipe-of-map PUBLIC ${CAF_INCLUDE} ${RANGE_INCLUDE} ./src)
target_link_libraries(pipe-of-map caf_pp ${CAF_LIB})

add_executable (divide-conquer examples/divide-conquer.cpp)
target_include_directories(divide-conquer PUBLIC ${CAF_INCLUDE} ${RANGE_INCLUDE} ./src)
target_link_libraries(divide-conquer caf_pp ${CAF_LIB})

# Install Library
install(TARGETS caf_pp DESTINATION lib)
install(DIRECTORY src/
        DESTINATION include/caf-pp
        FILES_MATCHING PATTERN "*.hpp")
