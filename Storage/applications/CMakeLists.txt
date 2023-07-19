
cmake_minimum_required(VERSION 3.2)

project(Applications)

file(GLOB SOURCES_CLIENT ./Client.cpp)
file(GLOB SOURCES_SERVER ./Server.cpp)

set(ASIO_INCLUDE ./../../Externals/asio/asio/include)

add_executable(Client ${SOURCES_CLIENT})
include_directories(Client ${ASIO_INCLUDE})
#link_libraries(Client Storage CommandLine)

add_executable(Server ${SOURCES_SERVER})
include_directories(Server ${ASIO_INCLUDE})
#link_libraries(Server Storage CommandLine)
