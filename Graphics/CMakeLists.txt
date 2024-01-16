project(Graphics)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ${GLEW_INCLUDE_DIR})
set(BINDINGS_LIBS glfw Imgui Nanovg ${GLEW_LIB})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_include_directories(${PROJECT_NAME} PRIVATE ${BINDINGS_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC Strings Math Utils)
target_link_libraries(${PROJECT_NAME} PRIVATE ${BINDINGS_LIBS})

### -------------------------- Examples  --------------------------  ###
add_executable(${PROJECT_NAME}Example ./examples/Example.cpp)
target_link_libraries(${PROJECT_NAME}Example ${PROJECT_NAME} ${BINDINGS_LIBS})
target_include_directories(${PROJECT_NAME}Example PRIVATE ${BINDINGS_INCLUDE})

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(${PROJECT_NAME}Tests ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} Utils)
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)
