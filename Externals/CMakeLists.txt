
add_subdirectory(glew/build/cmake/)
target_compile_definitions(glew_s PUBLIC GLEW_NO_GLU)

add_subdirectory(glfw)

project(Imgui)
set(${PROJECT_NAME}_SOURCES
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp

        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        )

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})
include_directories(${PROJECT_NAME} ./glfw/include)
target_include_directories(${PROJECT_NAME} PUBLIC ./imgui/ ./imgui/backends/)

project(Nanovg)
set(${PROJECT_NAME}_SOURCES
        nanovg/src/nanovg.c
        )

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ./nanovg/src/ ./nanovg/obsolete/)
