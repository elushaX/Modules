
cmake_minimum_required(VERSION 3.2)

project(Applications)

### ---------------------- Applications  ---------------------  ###
add_executable(Server ./Server.cpp)
add_executable(Client ./Client.cpp)

include_directories(Client ./../../Externals/asio/asio/include)
include_directories(Server ./../../Externals/asio/asio/include)