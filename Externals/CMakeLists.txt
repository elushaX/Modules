find_package(Vulkan REQUIRED)

if (MODULES_BUILD_DESKTOP)
	add_subdirectory(glfw)
endif ()

if (MODULES_BUILD_DESKTOP OR ANDROID)
	project(Imgui)
	set(${PROJECT_NAME}_SOURCES
			imgui/imgui.cpp
			imgui/imgui_draw.cpp
			imgui/imgui_tables.cpp
			imgui/imgui_widgets.cpp
			imgui/imgui_demo.cpp

			imgui/backends/imgui_impl_vulkan.cpp

			implot/implot.cpp
			implot/implot_items.cpp
	)

	add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})
	target_include_directories(${PROJECT_NAME} PUBLIC ./imgui/ ./imgui/backends/ ./implot/)
	target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
endif ()

if (ANDROID)
	add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
	target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
	target_link_libraries(native_app_glue PUBLIC android log)
endif ()

add_library(Picojson INTERFACE)
target_include_directories(Picojson INTERFACE ./picojson/)

project(Md4c)
add_library(${PROJECT_NAME} STATIC md4c/md4c.c)
target_include_directories(${PROJECT_NAME} PUBLIC ./md4c/)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

project(Nanovg)
add_library(${PROJECT_NAME} STATIC nanovg/src/nanovg.c)
target_include_directories(${PROJECT_NAME} PUBLIC ./nanovg/src/ ./nanovg/obsolete/)

project(NanovgVk)
add_library(${PROJECT_NAME} STATIC
		nanovg_vulkan/src/nanovg.c
		nanovg_vulkan/nanovg_vk_impl.c
)
target_include_directories(${PROJECT_NAME} PUBLIC ./nanovg_vulkan/src/ ./nanovg_vulkan/)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

project(Lua)
add_library(${PROJECT_NAME} STATIC "lua/onelua.c")
target_compile_definitions(${PROJECT_NAME} PUBLIC MAKE_LIB)
target_include_directories(${PROJECT_NAME} PUBLIC "./lua")
set_target_properties(${PROJECT_NAME} PROPERTIES
		C_STANDARD 99
		C_STANDARD_REQUIRED ON
)

project(ImageIO)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ./imageIO/)

if (MODULES_BUILD_TESTS)
	set(UNITTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unittest-cpp)
	file(GLOB UNITTEST_SOURCES ${UNITTEST_DIR}/UnitTest++/*.cpp)
	if (WIN32)
		file(GLOB UNITTEST_PLATFORM_SOURCES ${UNITTEST_DIR}/UnitTest++/Win32/*.cpp)
	else ()
		file(GLOB UNITTEST_PLATFORM_SOURCES ${UNITTEST_DIR}/UnitTest++/Posix/*.cpp)
	endif ()
	add_library(UnitTest++ STATIC ${UNITTEST_SOURCES} ${UNITTEST_PLATFORM_SOURCES})
	target_include_directories(UnitTest++ PUBLIC ${UNITTEST_DIR})

	add_subdirectory(googletest)
endif ()
