project(Sketch3D)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ${GLEW_INCLUDE_DIR})
set(BINDINGS_LIBS glfw Imgui ${GLEW_LIB})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE} ./ext/)
target_link_libraries(${PROJECT_NAME} PUBLIC Graphics Connection Widgets Math)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})

### -------------------------- Applications  --------------------------  ###
add_executable(Sketch3DApp ./applications/Entry.cpp)
target_link_libraries(Sketch3DApp ${PROJECT_NAME})
 
file(COPY "rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
file(COPY "applications/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
