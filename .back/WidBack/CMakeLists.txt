project(Widgets)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Math Graphics Imgui)

### -------------------------- Applications  --------------------------  ###

add_executable(SimpleGui examples/SimpleGUI.cpp)
target_link_libraries(SimpleGui ${PROJECT_NAME} ${GLEW_LIB})
target_include_directories(SimpleGui PUBLIC ../Externals/glfw/include ${GLEW_INCLUDE_DIR})

add_executable(ChatGui examples/ChatGUI.cpp)
target_link_libraries(ChatGui ${PROJECT_NAME} ${GLEW_LIB})
target_include_directories(ChatGui PUBLIC ../Externals/glfw/include ${GLEW_INCLUDE_DIR})

file(COPY "examples/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")