project(Graphics)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ${GLEW_INCLUDE_DIR})
set(BINDINGS_LIBS glfw Imgui Nanovg ${GLEW_LIB})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp")
file(GLOB HEADERS "./public/*.hpp")
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_include_directories(${PROJECT_NAME} PRIVATE ${BINDINGS_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC Math)
target_link_libraries(${PROJECT_NAME} PRIVATE ${BINDINGS_LIBS})

### -------------------------- Examples  --------------------------  ###
add_executable(Example${PROJECT_NAME} ./examples/Example.cpp)
target_link_libraries(Example${PROJECT_NAME} ${PROJECT_NAME} ${BINDINGS_LIBS})
target_include_directories(Example${PROJECT_NAME} PRIVATE ${BINDINGS_INCLUDE})


file(COPY "examples/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

