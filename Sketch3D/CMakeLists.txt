project(Sketch3Dlib)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ${GLEW_INCLUDE_DIR})
set(BINDINGS_LIBS glfw Imgui ${GLEW_LIB})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE} ./ext/)
target_link_libraries(${PROJECT_NAME} PUBLIC Graphics Connection Widgets Math Strings)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})

file(COPY "rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

### -------------------------- Applications  --------------------------  ###
add_executable(Sketch3D ./applications/Entry.cpp)
target_link_libraries(Sketch3D ${PROJECT_NAME})
 
