
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 23)

project(DataAnalysis)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ${EXTERNALS}/glfw/include ${EXTERNALS}/glew/include)
set(BINDINGS_LIBS glfw glew_s Imgui Nanovg)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Containers Allocators)

### -------------------------- Applications  --------------------------  ###
add_executable(NumRecTrain applications/NumRecTraining.cpp)
target_link_libraries(NumRecTrain ${PROJECT_NAME} Connection ImageIO)
file(COPY "applications/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

add_executable(NumRecApp applications/NumRecApp.cpp)
target_link_libraries(NumRecApp ${PROJECT_NAME} Connection ImageIO)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp" "./tests/*/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}Tests PUBLIC ./applications/)

target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lib)
