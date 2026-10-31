
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)

project(ModulesRoot)

include(cmake/ModulesOptions.txt)
include(cmake/DesktopEntry.cmake)

set(WINDOWS_LIBRARIES "../moduleswindowsl" CACHE STRING "Svn repository with windows libraries https://svn.riouxsvn.com/moduleswindowsl")

if (MODULES_BUILD_DESKTOP)
	include(cmake/FindGLEW.cmake)
	include(cmake/FindOIDN.cmake)
	include(cmake/FindPortAudio.cmake)
endif ()

add_subdirectory(Externals)

add_subdirectory(Modules)
add_subdirectory(Containers)
add_subdirectory(Math)
add_subdirectory(Callstack)
add_subdirectory(Diagnostics)
add_subdirectory(Allocators)
add_subdirectory(Strings)
add_subdirectory(Language)
add_subdirectory(Connection)
add_subdirectory(Graphics)
add_subdirectory(Widgets)
add_subdirectory(Taskboard)
add_subdirectory(3DScene)
if (MODULES_BUILD_DESKTOP OR ANDROID)
	add_subdirectory(RasterRender)
endif ()

if (MODULES_BUILD_DESKTOP)
	add_subdirectory(DataAnalysis)
	add_subdirectory(Objects)
	add_subdirectory(RayTracer)
endif ()
