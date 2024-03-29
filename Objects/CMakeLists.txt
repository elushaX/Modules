project(Objects)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC Math Connection LALR)
target_compile_definitions(${PROJECT_NAME} PUBLIC USERDATA_DESTROY_FAILED_TREE_ROOTS)

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
add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})