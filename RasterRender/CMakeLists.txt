project(RasterRender)

find_package(Vulkan REQUIRED)

find_program(GLSLC glslc HINTS Vulkan::glslc ${Vulkan_GLSLC_EXECUTABLE})
if(NOT GLSLC)
	set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
endif()

set(SHADER_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders_generated)
file(MAKE_DIRECTORY ${SHADER_GEN_DIR})

set(SHADER_INCS "")
foreach(SHADER default.vert default.frag solid.frag)
	set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER})
	set(SHADER_INC ${SHADER_GEN_DIR}/${SHADER}.inc)
	add_custom_command(
		OUTPUT ${SHADER_INC}
		COMMAND ${GLSLC} -O --target-env=vulkan1.0 -mfmt=c ${SHADER_SRC} -o ${SHADER_INC}
		DEPENDS ${SHADER_SRC}
		COMMENT "Compiling ${SHADER} to SPIR-V"
		VERBATIM
	)
	list(APPEND SHADER_INCS ${SHADER_INC})
endforeach()
add_custom_target(RasterRenderShaders DEPENDS ${SHADER_INCS})

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
add_dependencies(${PROJECT_NAME} RasterRenderShaders)

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ./ext/)
target_include_directories(${PROJECT_NAME} PRIVATE ./private/ ${SHADER_GEN_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Math Connection 3DScene Graphics Vulkan::Vulkan)