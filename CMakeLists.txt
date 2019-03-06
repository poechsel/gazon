cmake_minimum_required (VERSION 2.8.10)
project(gazon)

add_definitions(-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=gnu11)
set(COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(server
  include/grass.h
  include/common.h
  src/grass.cpp
  src/server.cpp
)

add_executable(client
  include/grass.h
  include/common.h
  src/grass.cpp
  src/client.cpp
)

message(${CMAKE_BINARY_DIR})
# Force colored output for the ninja generator
if (CMAKE_GENERATOR STREQUAL "Ninja")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
  endif()
endif()
