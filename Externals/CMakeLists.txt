
#add_subdirectory(glew/build/cmake/)
#target_compile_definitions(glew_s PUBLIC GLEW_NO_GLU)

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

project(Lua)

# Add the Lua source files
#file(GLOB ${PROJECT_NAME}_SOURCES "./lua/luaone.c")
# list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX "./lua/lua.c$")

# Create the Lua library
add_library(${PROJECT_NAME} STATIC "lua/onelua.c")

target_compile_definitions(${PROJECT_NAME} PUBLIC MAKE_LIB)

# Include directories for Lua headers
target_include_directories(${PROJECT_NAME} PUBLIC "./lua")

# Set compilation options, e.g., for compilation with C99
set_target_properties(${PROJECT_NAME} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
)

project(ImageIO)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ./imageIO/)
