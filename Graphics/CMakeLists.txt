project(Graphics)

find_package(Vulkan REQUIRED)

### ---------------------- Input Model (no platform)  ---------------------  ###
set(INPUT_SOURCES ./private/InputCoalescer.cpp ./private/InputRecording.cpp ./private/KeyMapping.cpp ./private/PlatformInfo.cpp ./private/SessionRecording.cpp ./private/TouchGesture.cpp)
add_library(${PROJECT_NAME}Input STATIC ${INPUT_SOURCES}
	./public/InputCodes.hpp ./public/PointerSample.hpp ./public/WindowInput.hpp ./public/InputCoalescer.hpp
	./public/InputRecording.hpp ./public/KeyMapping.hpp ./public/PlatformInfo.hpp ./public/SessionRecording.hpp ./public/TouchGesture.hpp)
target_include_directories(${PROJECT_NAME}Input PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME}Input PUBLIC Math)

### ---------------------- Vulkan Core (no platform)  ---------------------  ###
set(VULKAN_CORE_SOURCES
	./private/VulkanContext.cpp ./private/GpuTimer.cpp ./private/RenderTargetRegistry.cpp
	./private/WindowFrame.cpp ./private/EventHandler.cpp ./private/WindowInputDispatcher.cpp)
add_library(${PROJECT_NAME}Vulkan STATIC ${VULKAN_CORE_SOURCES}
	./public/VulkanContext.hpp ./public/GpuTimer.hpp ./public/RenderTargetRegistry.hpp
	./public/WindowFrame.hpp ./public/EventHandler.hpp)
target_include_directories(${PROJECT_NAME}Vulkan PUBLIC ./public/ PRIVATE ./private/)
target_link_libraries(${PROJECT_NAME}Vulkan PUBLIC Math Modules ${PROJECT_NAME}Input Vulkan::Vulkan)

if (MODULES_BUILD_DESKTOP)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client xkbcommon)
	pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
	pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

	### ---------------------- Wayland Protocols  ---------------------  ###
	set(WAYLAND_PROTOCOL_FILES
		stable/xdg-shell/xdg-shell.xml
		stable/viewporter/viewporter.xml
		staging/fractional-scale/fractional-scale-v1.xml
		staging/cursor-shape/cursor-shape-v1.xml
		staging/xdg-activation/xdg-activation-v1.xml
		stable/tablet/tablet-v2.xml
		unstable/text-input/text-input-unstable-v3.xml
		unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
		unstable/pointer-gestures/pointer-gestures-unstable-v1.xml
	)

	set(WAYLAND_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/wayland)
	file(MAKE_DIRECTORY ${WAYLAND_GENERATED_DIR})

	foreach(PROTOCOL_FILE ${WAYLAND_PROTOCOL_FILES})
		get_filename_component(PROTOCOL_NAME ${PROTOCOL_FILE} NAME_WE)
		set(PROTOCOL_XML ${WAYLAND_PROTOCOLS_DIR}/${PROTOCOL_FILE})
		set(PROTOCOL_HEADER ${WAYLAND_GENERATED_DIR}/${PROTOCOL_NAME}-client-protocol.h)
		set(PROTOCOL_SOURCE ${WAYLAND_GENERATED_DIR}/${PROTOCOL_NAME}-protocol.c)

		add_custom_command(
			OUTPUT ${PROTOCOL_HEADER} ${PROTOCOL_SOURCE}
			COMMAND ${WAYLAND_SCANNER} client-header ${PROTOCOL_XML} ${PROTOCOL_HEADER}
			COMMAND ${WAYLAND_SCANNER} private-code ${PROTOCOL_XML} ${PROTOCOL_SOURCE}
			DEPENDS ${PROTOCOL_XML}
		)
		list(APPEND WAYLAND_PROTOCOL_SOURCES ${PROTOCOL_HEADER} ${PROTOCOL_SOURCE})
	endforeach()

	add_library(WaylandProtocols STATIC ${WAYLAND_PROTOCOL_SOURCES})
	target_include_directories(WaylandProtocols PUBLIC ${WAYLAND_GENERATED_DIR})
	target_link_libraries(WaylandProtocols PUBLIC PkgConfig::WAYLAND)
	set_target_properties(WaylandProtocols PROPERTIES LINKER_LANGUAGE C)
endif ()

### ---------------------- Window Library  ---------------------  ###
file(GLOB SOURCES CONFIGURE_DEPENDS "./private/*.cpp")
foreach(SHARED_SOURCE ${INPUT_SOURCES} ${VULKAN_CORE_SOURCES})
	get_filename_component(SHARED_PATH ${SHARED_SOURCE} ABSOLUTE)
	list(REMOVE_ITEM SOURCES ${SHARED_PATH})
endforeach()
file(GLOB WAYLAND_SOURCES CONFIGURE_DEPENDS "./private/Wayland*.cpp")
file(GLOB ANDROID_SOURCES CONFIGURE_DEPENDS "./private/android/*.cpp")
list(REMOVE_ITEM SOURCES ${WAYLAND_SOURCES})
file(GLOB HEADERS "./public/*.hpp")

if (ANDROID)
	add_library(${PROJECT_NAME} STATIC ${SOURCES} ${ANDROID_SOURCES} ${HEADERS})
	target_link_libraries(${PROJECT_NAME} PRIVATE Imgui NanovgVk Vulkan::Vulkan android log)
	target_link_libraries(${PROJECT_NAME} PUBLIC native_app_glue)
elseif (MODULES_BUILD_DESKTOP)
	add_library(${PROJECT_NAME} STATIC ${SOURCES} ${WAYLAND_SOURCES} ${HEADERS})
	target_link_libraries(${PROJECT_NAME} PRIVATE Imgui NanovgVk Vulkan::Vulkan WaylandProtocols PkgConfig::WAYLAND)
endif ()

if (TARGET ${PROJECT_NAME})
	target_include_directories(${PROJECT_NAME} PUBLIC ./public/ PRIVATE ./private/)
	target_link_libraries(${PROJECT_NAME} PUBLIC Math Diagnostics ${PROJECT_NAME}Input ${PROJECT_NAME}Vulkan)
	target_compile_definitions(${PROJECT_NAME} PRIVATE MODULES_GENERIC_APP_ID="${MODULES_GENERIC_APP_ID}" MODULES_GENERIC_APP_NAME="${MODULES_GENERIC_APP_NAME}")
	file(COPY "../Graphics/rsc" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
endif ()

### -------------------------- Examples  --------------------------  ###
if (MODULES_BUILD_DESKTOP AND MODULES_BUILD_EXAMPLES)
	add_executable(Example${PROJECT_NAME} ./examples/Example.cpp)
	target_link_libraries(Example${PROJECT_NAME} ${PROJECT_NAME} Imgui)

	add_executable(OffscreenProof ./examples/OffscreenProof.cpp)
	target_link_libraries(OffscreenProof NanovgVk Vulkan::Vulkan)
endif ()

### -------------------------- Tests  --------------------------  ###
if (MODULES_BUILD_TESTS)
	add_executable(Test${PROJECT_NAME} ./tests/Main.cpp ./tests/WindowFrameTests.cpp ./tests/KeyMappingTests.cpp ./tests/NanovgTextureSlotTests.cpp ./tests/InputCoalescerTests.cpp ./tests/InputRecordingTests.cpp ./tests/PlatformInfoTests.cpp ./tests/TouchGestureTests.cpp ./tests/FontCoverageTests.cpp ./private/WindowFrame.cpp)
	target_include_directories(Test${PROJECT_NAME} PRIVATE ./public/ ./private/)
	target_compile_definitions(Test${PROJECT_NAME} PRIVATE MODULES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
	target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME}Input UnitTest++ NanovgVk)
	add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})
endif ()
