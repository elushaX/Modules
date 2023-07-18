
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)

project(Storage)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Strings)

### ---------------------- Applications  ---------------------  ###
add_executable(Server ./applications/Server.cpp)
add_executable(Client ./applications/Client.cpp)
target_link_libraries(Server PUBLIC ${PROJECT_NAME})
target_link_libraries(Client PUBLIC ${PROJECT_NAME})

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests  ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib)