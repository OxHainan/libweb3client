cmake_minimum_required(VERSION 3.10)
project(web3client VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.79.0 REQUIRED log chrono system filesystem thread iostreams)

find_package(OpenSSL REQUIRED)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/exported)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/exported/intx/include)

file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(websocket ${SRCS})
target_link_libraries(websocket PUBLIC Boost::system Boost::filesystem
Boost::log Boost::chrono Boost::iostreams Boost::thread OpenSSL::SSL OpenSSL::Crypto)

add_executable(client sample/client.cpp)
target_link_libraries(client PUBLIC websocket)
