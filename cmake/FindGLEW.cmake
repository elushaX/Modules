if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_PREFIX_PATH  "${WINDOWS_LIBRARIES}/glew-2.1.0")
	
	find_package(GLEW REQUIRED)
	set(GLEW_LIB ${GLEW_STATIC_LIBRARY_RELEASE} opengl32.lib CACHE STRING "Path to PortAudio includes")

else()
	find_package(GLEW REQUIRED)
	set(GLEW_LIB ${GLEW_LIBRARIES} GL CACHE STRING "Path to PortAudio includes")
endif()