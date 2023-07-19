
cmake_minimum_required(VERSION 3.2)

project(Applications)

file(GLOB SOURCES_CLIENT ./Client.cpp ./SystemAPI.cpp)
file(GLOB SOURCES_SERVER ./Server.cpp ./SystemAPI.cpp)

add_executable(Client ${SOURCES_CLIENT})
target_link_libraries(Client Storage CommandLine)

add_executable(Server ${SOURCES_SERVER})
target_link_libraries(Server Storage CommandLine)
