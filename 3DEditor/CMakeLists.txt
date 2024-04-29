project(3DEditor)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ../Externals)
set(BINDINGS_LIBS glfw Imgui)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE} ./ext/)
target_link_libraries(${PROJECT_NAME} PUBLIC Graphics Connection Widgets Math RasterRender)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})

### -------------------------- Applications  --------------------------  ###
add_executable(3DEditorApp ./applications/Entry.cpp ./applications/SceneLoad.cpp)
target_link_libraries(3DEditorApp ${PROJECT_NAME} Lua ImageIO)

file(COPY "rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
file(COPY "rsc/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
