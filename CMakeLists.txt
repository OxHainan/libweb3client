cmake_minimum_required(VERSION 3.10)
project(web3client VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.79.0 REQUIRED thread)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/exported)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/exported/intx/include)

file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
set(Boost_USE_STATIC_LIBS ON)

add_library(websocket STATIC ${SRCS})
target_link_libraries(websocket PUBLIC Boost::thread)

add_executable(client sample/client.cpp)
target_link_libraries(client PUBLIC websocket)
