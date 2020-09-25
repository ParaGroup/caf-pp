cmake_minimum_required(VERSION 3.0.0)
project(caf-pp VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

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
find_path(RANGE_INCLUDE range/v3/all.hpp ${CMAKE_CURRENT_SOURCE_DIR}/external/range-v3/include)
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
  install(DIRECTORY ${RANGE_INCLUDE} DESTINATION .)

  # for convenience setup a target
  add_library(range INTERFACE)
  target_include_directories(range INTERFACE
                            $<BUILD_INTERFACE:${RANGE_INCLUDE}>
                            $<INSTALL_INTERFACE:include>)
  
  # need to export target as well
  install(TARGETS range EXPORT my_export_set DESTINATION include)
endif()

if(${SIZED_QUEUE})
  add_compile_definitions(SIZED_QUEUE)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

# Library
add_library(caf_pp src/patterns.cpp
                   src/next.cpp
                   src/pp_actor.cpp)
target_include_directories(caf_pp PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(caf_pp ${CAF_LIB} pthread)

# Targets
add_executable (pipe-of-farm examples/pipe-of-farm.cpp)
target_include_directories(pipe-of-farm PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(pipe-of-farm caf_pp ${CAF_LIB})

add_executable (pipe-of-map examples/pipe-of-map.cpp)
target_include_directories(pipe-of-map PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(pipe-of-map caf_pp ${CAF_LIB})

add_executable (quicksort examples/quicksort.cpp)
target_include_directories(quicksort PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(quicksort caf_pp ${CAF_LIB})

add_executable (recMM examples/recMM.cpp)
target_include_directories(recMM PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(recMM caf_pp ${CAF_LIB})

add_executable (routerfarm examples/routerfarm.cpp)
target_include_directories(routerfarm PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(routerfarm caf_pp ${CAF_LIB})

add_executable (pipe-of-farm2 examples/pipe-of-farm2.cpp)
target_include_directories(pipe-of-farm2 PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
target_link_libraries(pipe-of-farm2 caf_pp ${CAF_LIB})

if(${SIZED_QUEUE})
  add_executable (max-rate-pipe examples/max-rate-pipe.cpp)
  target_include_directories(max-rate-pipe PUBLIC ./src ${RANGE_INCLUDE} ${CAF_INCLUDE})
  target_link_libraries(max-rate-pipe caf_pp ${CAF_LIB})
endif()

# Install Library
install(TARGETS caf_pp DESTINATION lib)
install(DIRECTORY src/
        DESTINATION include/caf-pp
        FILES_MATCHING PATTERN "*.hpp")
