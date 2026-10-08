function(sketch2d_library name dir)
	file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/private/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/public/*.hpp)
	add_library(${name} STATIC ${sources})
	target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/public)
endfunction()

sketch2d_library(Sketch2DCommon common)
target_link_libraries(Sketch2DCommon PUBLIC Math Containers Modules)

sketch2d_library(Sketch2DBrush brush)
target_link_libraries(Sketch2DBrush PUBLIC Sketch2DCommon)

sketch2d_library(Sketch2DEngine engine)
target_link_libraries(Sketch2DEngine PUBLIC Sketch2DBrush)
set(SKETCH2D_TILE_SIZE 128 CACHE STRING "Edge length of a canvas tile in pixels (power of two, at least 64)")
target_compile_definitions(Sketch2DEngine PUBLIC SKETCH2D_TILE_SIZE=${SKETCH2D_TILE_SIZE})

sketch2d_library(Sketch2DDocument document)
target_link_libraries(Sketch2DDocument PUBLIC Sketch2DEngine)

sketch2d_library(Sketch2DLibrary library)
target_link_libraries(Sketch2DLibrary PUBLIC Sketch2DDocument Graphics)

sketch2d_library(Sketch2DInterface ui)
target_link_libraries(Sketch2DInterface PUBLIC Sketch2DLibrary Widgets)

add_library(Sketch2DUI INTERFACE)
target_link_libraries(Sketch2DUI INTERFACE Sketch2DInterface)

if (MODULES_BUILD_DESKTOP OR ANDROID)
	find_package(Vulkan REQUIRED)
	find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE})
	if(NOT GLSLC)
		set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
	endif()
	set(SHADER_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders_generated)
	file(MAKE_DIRECTORY ${SHADER_GEN_DIR})
	set(SHADER_INCS "")
	foreach(SHADER tile.vert tile.frag solid.vert solid.frag)
		add_custom_command(
			OUTPUT ${SHADER_GEN_DIR}/${SHADER}.inc
			COMMAND ${GLSLC} -O --target-env=vulkan1.0 -mfmt=c
					${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${SHADER} -o ${SHADER_GEN_DIR}/${SHADER}.inc
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${SHADER}
			COMMENT "Compiling ${SHADER} to SPIR-V"
			VERBATIM
		)
		list(APPEND SHADER_INCS ${SHADER_GEN_DIR}/${SHADER}.inc)
	endforeach()
	add_custom_target(sketch2dShaders DEPENDS ${SHADER_INCS})

	sketch2d_library(Sketch2D render)
	add_dependencies(Sketch2D sketch2dShaders)
	target_include_directories(Sketch2D PRIVATE ${SHADER_GEN_DIR})
	target_link_libraries(Sketch2D PUBLIC Sketch2DInterface Graphics RasterRender Vulkan::Vulkan)

	sketch2d_library(Sketch2DApplication app)
	target_link_libraries(Sketch2DApplication PUBLIC Sketch2D WidgetsApp)
	execute_process(COMMAND git rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE SKETCH2D_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	target_compile_definitions(Sketch2DApplication PRIVATE SKETCH2D_REVISION="${SKETCH2D_REVISION}")
endif ()
