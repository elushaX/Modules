
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)

project(Utils)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)

### ----------------------  Dependencies   ---------------------  ###
target_link_libraries(${PROJECT_NAME} PUBLIC Containers)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests  ${PROJECT_NAME})
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)


install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib)