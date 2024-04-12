project(RasterRender)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ${GLEW_INCLUDE_DIR})
set(BINDINGS_LIBS ${GLEW_LIB})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE} ./ext/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Connection)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})