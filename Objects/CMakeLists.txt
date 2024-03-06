project(Objects)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/lalr/src)
# set(BINDINGS_LIBS )

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC Strings Math Connection)

### -------------------------- Applications  --------------------------  ###
#add_executable(osc ./applications/Compiler.cpp)
#add_executable(osi ./applications/Interpreter.cpp)
add_executable(osg ./applications/GUI.cpp ./applications/GUIEntry.cpp)

#target_link_libraries(osc ${PROJECT_NAME})
#target_link_libraries(osi ${PROJECT_NAME})
target_link_libraries(osg ${PROJECT_NAME} Graphics Imgui)

file(COPY "rsc/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

### -------------------------- Tests  --------------------------  ###
file(GLOB TEST_SOURCES "./tests/*.cpp" "./tests/*/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)