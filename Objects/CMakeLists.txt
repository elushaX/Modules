
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)

project(Objects)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Strings Math Tokenizer CommandLine Connection)

### -------------------------- Applications  --------------------------  ###
#add_executable(osc ./applications/Compiler.cpp)
#add_executable(osi ./applications/Interpreter.cpp)

#target_link_libraries(osc ${PROJECT_NAME})
#target_link_libraries(osi ${PROJECT_NAME})

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp" "./tests/*/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests  ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib)