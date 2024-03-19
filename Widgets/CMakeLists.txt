project(Widgets)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Graphics Strings)

### -------------------------- Tests  --------------------------  ###
enable_testing()
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(Test${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME} UnitTest++)
add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})

### -------------------------- Applications  --------------------------  ###

add_executable(ExampleGui examples/Entry.cpp)
target_link_libraries(ExampleGui ${PROJECT_NAME} Graphics Imgui Nanovg glfw ${GLEW_LIB})
target_include_directories(ExampleGui PUBLIC ../Externals/glfw/include ${GLEW_INCLUDE_DIR})
file(COPY "examples/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")