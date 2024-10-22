#set(OIDN_DEVICE_CPU ON)
#set(OIDN_STATIC_LIB ON)
#add_subdirectory(oidn)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CMAKE_PREFIX_PATH  "${WINDOWS_LIBRARIES}/oidn-2.3.0.x64.windows")
endif()

find_package(OpenImageDenoise REQUIRED)



set(TARGETS_LIST OpenImageDenoise)

foreach(TARGET ${TARGETS_LIST})
  if(TARGET ${TARGET})
    message("Found ${TARGET}.")
  else()
    message(ERROR "Not Found ${TARGET}.")
  endif()
endforeach()