project(LibraryViewer)

### ---------------------- Static Library  ---------------------  ###
file(GLOB SOURCES "./private/*.cpp" "./private/*/*.cpp")
file(GLOB HEADERS "./public/*.hpp" "./public/*/*.hpp" "./applications/*.hpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ./public/)
target_link_libraries(${PROJECT_NAME} PUBLIC Graphics Connection)

file(COPY "library" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")
file(COPY "applications/Font.ttf" DESTINATION "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/")

### -------------------------- Applications  --------------------------  ###
add_executable(libView ./applications/Entry.cpp)
target_link_libraries(libView ${PROJECT_NAME} LibraryViewer)