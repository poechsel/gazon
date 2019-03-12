cmake_minimum_required (VERSION 2.8.10)
project(gazon)

add_definitions(-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -pthread -std=c++11)
set(COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)

file(GLOB_RECURSE COMMONSH ${PROJECT_SOURCE_DIR}/src/common/*.h)
file(GLOB_RECURSE CLIENTH ${PROJECT_SOURCE_DIR}/src/client/*.h)
file(GLOB_RECURSE SERVERH ${PROJECT_SOURCE_DIR}/src/server/*.h)
file(GLOB_RECURSE COMMONSCPP ${PROJECT_SOURCE_DIR}/src/common/*.cpp)
file(GLOB_RECURSE CLIENTCPP ${PROJECT_SOURCE_DIR}/src/client/*.cpp)
file(GLOB_RECURSE SERVERCPP ${PROJECT_SOURCE_DIR}/src/server/*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common
  ${CMAKE_CURRENT_SOURCE_DIR}/include/client
  ${CMAKE_CURRENT_SOURCE_DIR}/include/server
)

add_executable(server
  ${COMMONSH}
  ${SERVERH}
  ${COMMONSCPP}
  ${SERVERCPP}
)

add_executable(client
  ${COMMONSH}
  ${CLIENTH}
  ${COMMONSCPP}
  ${CLIENTCPP}
)

TARGET_LINK_LIBRARIES( server
  pthread
)
TARGET_LINK_LIBRARIES( client
  pthread
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
