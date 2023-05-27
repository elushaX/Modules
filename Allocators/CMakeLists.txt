
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)

project(Allocator)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Utils)

### -------------------------- Tests  --------------------------  ###
enable_testing()
add_executable(${PROJECT_NAME}Tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/Tests.cpp)
target_link_libraries(${PROJECT_NAME}Tests  ${PROJECT_NAME})
add_test(${PROJECT_NAME} ${PROJECT_NAME}Tests)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib)