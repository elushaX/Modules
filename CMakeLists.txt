
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/./tmp/install)
# set(CMAKE_CXX_FLAGS "-fuse-ld=lld-15 ${CMAKE_CXX_FLAGS} -gdwarf-4 ")

project(ModulesRoot)

include(cmake/ModulesOptions.txt)

set(WINDOWS_LIBRARIES "../moduleswindowsl" CACHE STRING "Svn repository with windows libraries https://svn.riouxsvn.com/moduleswindowsl")

include(cmake/findGLEW.cmake)
include(cmake/findOIDN.cmake)
include(cmake/findPortAudio.cmake)

add_subdirectory(Externals)

add_subdirectory(Modules)
add_subdirectory(Containers)
add_subdirectory(Math)
# add_subdirectory(Language)
add_subdirectory(Connection)
add_subdirectory(Graphics)
add_subdirectory(DataAnalysis)
add_subdirectory(Objects)
add_subdirectory(Widgets)
add_subdirectory(LibraryViewer)
add_subdirectory(RasterRender)
add_subdirectory(3DScene)
add_subdirectory(RayTracer)
add_subdirectory(Sketch3D)
add_subdirectory(3DEditor)
