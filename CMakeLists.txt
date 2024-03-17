
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/./tmp/install)
# set(CMAKE_CXX_FLAGS "-fuse-ld=lld-15 ${CMAKE_CXX_FLAGS} -gdwarf-4 ")
add_compile_definitions(MEM_DEBUG)

project(ModulesRoot)

include(CMakeOptions.txt)

add_subdirectory(Modules)
add_subdirectory(Callstack)
add_subdirectory(Containers)
add_subdirectory(Math)
add_subdirectory(Strings)
add_subdirectory(Allocators)
# add_subdirectory(Language)
add_subdirectory(Externals)
add_subdirectory(Connection)
add_subdirectory(Graphics)
add_subdirectory(RayTracer)
add_subdirectory(DataAnalysis)
add_subdirectory(Objects)
add_subdirectory(Widgets)
add_subdirectory(LibraryViewer)
add_subdirectory(Sketch3D)