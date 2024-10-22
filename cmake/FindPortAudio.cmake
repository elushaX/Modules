if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # relative paths
  set(RELATIVE_INCLUDE_DIR  "${WINDOWS_LIBRARIES}/portaudio/include")
  set(RELATIVE_LIB  "${WINDOWS_LIBRARIES}/portaudio_build/Debug/portaudio.lib")

  # Convert to absolute paths
  get_filename_component(ABSOLUTE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RELATIVE_INCLUDE_DIR}" ABSOLUTE)
  get_filename_component(ABSOLUTE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/${RELATIVE_LIB}" ABSOLUTE)

  # Set absolute paths with respect to the parent scope
  set(PORTAUDIO_INCLUDE_DIR "${ABSOLUTE_INCLUDE_DIR}" CACHE STRING "Path to PortAudio includes")
  set(PORTAUDIO_LIB "${ABSOLUTE_LIB}" CACHE STRING "Path to PortAudio library")

else()
  set(PORTAUDIO_LIB portaudio CACHE STRING "Path to PortAudio library")
endif()