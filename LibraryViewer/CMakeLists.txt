project(LibraryViewer)

find_package(ALSA REQUIRED)
# find_package(PortAudio REQUIRED)

### ---------------------- Externals  ---------------------  ###
set(BINDINGS_INCLUDE ../Externals/glfw/include ../Externals/glew/include)
set(BINDINGS_LIBS glfw glew_s Imgui Nanovg portaudio)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/ ${BINDINGS_INCLUDE} ./ext/)
target_link_libraries(${PROJECT_NAME} PUBLIC Graphics Connection)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BINDINGS_LIBS})

file(COPY "library" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
file(COPY "applications/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

### -------------------------- Applications  --------------------------  ###
add_executable(libView ./applications/Entry.cpp)
target_link_libraries(libView ${PROJECT_NAME} ${ALSA_LIBRARY})